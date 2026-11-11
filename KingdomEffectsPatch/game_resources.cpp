#define WIN32_LEAN_AND_MEAN
#include "game_resources.hpp"
#include <algorithm>
#include <array>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <list>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
#include <cwctype>

namespace kingdom {
namespace {
struct ResourceCard {
    const char *id, *art, *top, *bottom, *left, *right, *emblem;
    const wchar_t *title, *rules, *types;
    int coins, potions, debt;
    bool landscape, dark;
    const char* value_symbol;
    const wchar_t* value;
};
struct ResourceAlias { const char *name, *path; };
#include "resource_catalog.generated.hpp"
constexpr std::size_t budget = 96u * 1024u * 1024u;
constexpr std::size_t max_pixels = 16u * 1024u * 1024u;
void require(bool ok) { if (!ok) throw std::runtime_error("Invalid Dominion image resource"); }
template<class T> T number(const std::vector<unsigned char>& data, std::size_t offset) {
    require(offset <= data.size() && sizeof(T) <= data.size() - offset);
    T value{}; std::memcpy(&value, data.data() + offset, sizeof value); return value;
}
unsigned be32(const unsigned char* data) {
    return (unsigned(data[0]) << 24) | (unsigned(data[1]) << 16) | (unsigned(data[2]) << 8) | data[3];
}
class Resources {
public:
    std::recursive_mutex mutex;
    ULONG_PTR graphics = 0;
    std::ifstream file;
    std::uint64_t file_size = 0;
    std::map<std::string, std::pair<unsigned,unsigned>, std::less<>> entries;
    struct Cached { ResourceImage image; std::size_t bytes; std::uint64_t used; };
    std::map<std::string,Cached,std::less<>> cache;
    std::size_t bytes = 0;
    std::uint64_t clock = 0;
    bool attempted = false;
    Resources() { Gdiplus::GdiplusStartupInput input; Gdiplus::GdiplusStartup(&graphics,&input,nullptr); }
    ~Resources() { cache.clear(); if (graphics) Gdiplus::GdiplusShutdown(graphics); }
    bool open(const std::filesystem::path& path) {
        file.close(); file.clear(); entries.clear(); cache.clear(); bytes=0; attempted=true;
        try {
            file.open(path,std::ios::binary); require(bool(file));
            file.seekg(0,std::ios::end); const auto end=file.tellg(); require(end>=12); file_size=std::uint64_t(end);
            file.seekg(0); std::vector<unsigned char> header(12); file.read(reinterpret_cast<char*>(header.data()),12);
            require(std::memcmp(header.data(),"KCPX",4)==0 && number<unsigned>(header,4)==7);
            const auto count=number<unsigned>(header,8);
            require(count <= 100000 && 12ull+168ull*count <= file_size);
            for (unsigned i=0;i<count;++i) {
                std::vector<unsigned char> row(168); file.read(reinterpret_cast<char*>(row.data()),168); require(bool(file));
                const auto offset=number<unsigned>(row,12), size=number<unsigned>(row,16);
                require(offset >= 12ull+168ull*count && std::uint64_t(offset)+size <= file_size);
                const auto zero=std::find(row.begin()+40,row.end(),0); require(zero!=row.end());
                // Build 3315 contains duplicate names; later entries override earlier ones.
                std::string name(row.begin()+40,zero); entries[name]=std::make_pair(offset,size);
            }
            return true;
        } catch (...) { entries.clear(); file.close(); return false; }
    }
    void initialize() {
        if (attempted) return;
        std::array<wchar_t,32768> path{};
        const auto length=GetModuleFileNameW(nullptr,path.data(),DWORD(path.size()));
        if (!length || length>=path.size()) { attempted=true; return; }
        open(std::filesystem::path(path.data()).parent_path()/L"Dominion.xpack");
    }
    std::vector<unsigned char> read(const std::string& key) {
        const auto found=entries.find(key); require(found!=entries.end());
        const auto [offset,size]=found->second; require(size<=64u*1024u*1024u);
        std::vector<unsigned char> data(size); file.clear(); file.seekg(offset);
        file.read(reinterpret_cast<char*>(data.data()),size); require(bool(file)); return data;
    }
    ResourceImage hit(const std::string& key) {
        const auto it=cache.find(key); if (it==cache.end()) return {};
        it->second.used=++clock; return it->second.image;
    }
    void room(std::size_t count) {
        require(count<=budget);
        while (bytes > budget-count) {
            auto oldest=cache.end();
            for (auto it=cache.begin();it!=cache.end();++it)
                if (it->second.image.use_count()==1 && (oldest==cache.end() || it->second.used<oldest->second.used)) oldest=it;
            require(oldest!=cache.end()); bytes-=oldest->second.bytes; cache.erase(oldest);
        }
    }
    ResourceImage put(const std::string& key, ResourceImage image) {
        const auto count=std::size_t(image->GetWidth())*image->GetHeight()*4;
        room(count); cache.emplace(key,Cached{image,count,++clock}); bytes+=count; return image;
    }
    ResourceImage load(std::string key, unsigned depth=0) {
        require(depth<8); initialize(); if (auto cached=hit(key)) return cached;
        const auto data=read(key); const auto start=number<std::uint64_t>(data,0x80);
        if (start<=data.size() && data.size()-std::size_t(start)>=4 && std::memcmp(data.data()+start,"qoif",4)==0) {
            require(start>=0x88);
            const auto begin=std::size_t(start); require(data.size()-begin>=22);
            const auto width=be32(data.data()+begin+4),height=be32(data.data()+begin+8);
            require(width && height && width<=8192 && height<=8192 && std::uint64_t(width)*height<=max_pixels);
            require((data[begin+12]==3 || data[begin+12]==4) && data[begin+13]<=1);
            const auto count=std::size_t(width)*height; room(count*4);
            std::vector<unsigned> pixels(count); std::array<std::array<unsigned char,4>,64> index{};
            std::array<unsigned char,4> p{0,0,0,255}; std::size_t cursor=begin+14;
            const auto next=[&]() { require(cursor<data.size()-8); return data[cursor++]; };
            unsigned run=0;
            for (std::size_t i=0;i<count;++i) {
                if (run) --run;
                else {
                    const auto tag=next();
                    if (tag==0xfe) { p[0]=next(); p[1]=next(); p[2]=next(); }
                    else if (tag==0xff) { p[0]=next(); p[1]=next(); p[2]=next(); p[3]=next(); }
                    else switch (tag>>6) {
                        case 0: p=index[tag&63]; break;
                        case 1: p[0]=static_cast<unsigned char>(p[0]+((tag>>4)&3)-2);
                                p[1]=static_cast<unsigned char>(p[1]+((tag>>2)&3)-2);
                                p[2]=static_cast<unsigned char>(p[2]+(tag&3)-2); break;
                        case 2: { const int green=(tag&63)-32; const auto diff=next();
                                  p[0]=static_cast<unsigned char>(p[0]+green+(diff>>4)-8);
                                  p[1]=static_cast<unsigned char>(p[1]+green);
                                  p[2]=static_cast<unsigned char>(p[2]+green+(diff&15)-8); break; }
                        case 3: run=tag&63; require(run<count-i); break;
                    }
                    index[(p[0]*3+p[1]*5+p[2]*7+p[3]*11)%64]=p;
                }
                // Dominion's QOI channels are B,G,R,A rather than R,G,B,A.
                pixels[i]=(unsigned(p[3])<<24)|(unsigned(p[2])<<16)|(unsigned(p[1])<<8)|p[0];
            }
            constexpr unsigned char endmark[]{0,0,0,0,0,0,0,1};
            require(!run && cursor+8<=data.size() && std::memcmp(data.data()+cursor,endmark,8)==0);
            auto image=std::make_shared<Gdiplus::Bitmap>(width,height,PixelFormat32bppARGB);
            require(image->GetLastStatus()==Gdiplus::Ok);
            Gdiplus::BitmapData locked{}; Gdiplus::Rect rect(0,0,INT(width),INT(height));
            require(image->LockBits(&rect,Gdiplus::ImageLockModeWrite,PixelFormat32bppARGB,&locked)==Gdiplus::Ok);
            for (unsigned y=0;y<height;++y) std::memcpy(static_cast<unsigned char*>(locked.Scan0)+std::ptrdiff_t(y)*locked.Stride,pixels.data()+std::size_t(y)*width,width*4);
            image->UnlockBits(&locked); return put(key,std::move(image));
        }
        const auto width=number<unsigned>(data,0x28),height=number<unsigned>(data,0x2c);
        const auto x=number<unsigned>(data,0x5c),y=number<unsigned>(data,0x60);
        require(data.size()>0x78); const auto zero=std::find(data.begin()+0x78,data.end(),0); require(zero!=data.end());
        std::string atlas(data.begin()+0x78,zero);
        constexpr std::string_view suffix=".texture";
        if (atlas.size()>=suffix.size() && atlas.compare(atlas.size()-suffix.size(),suffix.size(),suffix)==0) atlas.resize(atlas.size()-suffix.size());
        auto texture=load(atlas,depth+1);
        require(width && height && x<=texture->GetWidth() && y<=texture->GetHeight() &&
                width<=texture->GetWidth()-x && height<=texture->GetHeight()-y);
        room(std::size_t(width)*height*4);
        ResourceImage image(texture->Clone(INT(x),INT(y),INT(width),INT(height),PixelFormat32bppARGB));
        require(image && image->GetLastStatus()==Gdiplus::Ok); return put(key,std::move(image));
    }
};
Resources& resources() {
    // The injected DLL remains loaded for the process lifetime. Avoid running
    // GDI+ shutdown from a CRT static destructor under Windows' loader lock.
    // The bounded cache and token are reclaimed by the OS with the process.
    static Resources* const value = new Resources;
    return *value;
}
ResourceImage safe_load(Resources& store,const std::string& path) {
    if (path.empty()) return {};
    try { return store.load(path); } catch (...) { return {}; }
}

// Draw the same portrait/landscape frames and localized rich text as the generator.
struct Painter {
    Resources& store;
    Gdiplus::Graphics& graphics;
    void place(const std::string& path,float x,float y,float w,float h,bool flip=false) {
        if (auto image=safe_load(store,path)) {
            if (flip) {
                Gdiplus::PointF points[]{ {x+w,y},{x,y},{x+w,y+h} };
                graphics.DrawImage(image.get(),points,3,0,0,float(image->GetWidth()),float(image->GetHeight()),Gdiplus::UnitPixel);
            } else graphics.DrawImage(image.get(),x,y,w,h);
        }
    }
    void shared(const char* name,float x,float y,float w,float h) { place(std::string("res/card_gen/shared/")+name,x,y,w,h); }
    float text_width(const std::wstring& text,float size,bool bold=false) {
        Gdiplus::Font font(L"Times New Roman",size,bold?Gdiplus::FontStyleBold:Gdiplus::FontStyleRegular,Gdiplus::UnitPixel);
        Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());
        format.SetFormatFlags(format.GetFormatFlags()|Gdiplus::StringFormatFlagsMeasureTrailingSpaces);
        Gdiplus::RectF bounds; graphics.MeasureString(text.c_str(),INT(text.size()),&font,Gdiplus::PointF(0,0),&format,&bounds); return bounds.Width;
    }
    void text(const std::wstring& value,float x,float y,float size,bool bold,Gdiplus::Color ink) {
        Gdiplus::Font font(L"Times New Roman",size,bold?Gdiplus::FontStyleBold:Gdiplus::FontStyleRegular,Gdiplus::UnitPixel);
        Gdiplus::SolidBrush brush(ink); graphics.DrawString(value.c_str(),INT(value.size()),&font,Gdiplus::PointF(x,y),Gdiplus::StringFormat::GenericTypographic(),&brush);
    }
    void centered(std::wstring value,float x,float y,float size,float width,Gdiplus::Color ink) {
        if (!value.empty()) CharUpperBuffW(value.data(),DWORD(value.size()));
        while (size>10 && text_width(value,size)>width) --size;
        text(value,x-text_width(value,size)/2,y-size*.62f,size,false,ink);
    }
    void symbol(const std::string& name,const std::wstring& label,float x,float y,float diameter) {
        const auto key=name=="coin"?"ico_coin_smaller":name=="debt"?"ico_debt":name=="potion"?"ico_potion":name=="sun"?"ico_sun":"vp";
        shared(key,x,y,diameter,diameter);
        if (!label.empty()) {
            const auto size=diameter*(label.size()>1?.62f:.81f);
            text(label,x+(diameter-text_width(label,size,true))/2,y+(diameter-size)*.38f,size,true,
                name=="coin" || name=="sun"?Gdiplus::Color(255,22,18,14):Gdiplus::Color(255,255,253,243));
        }
    }
    struct Fragment { std::wstring text; std::string symbol; bool bold; float width; };
    struct Row { std::vector<Fragment> parts; float width=0,height=0; bool separator=false; };
    std::vector<Row> layout(const std::wstring& rules,float size,float width) {
        std::vector<Row> rows; Row row; row.height=size+3; bool bold=false; std::size_t at=0;
        const auto newline=[&]() { rows.push_back(row); row=Row{}; row.height=size+3; };
        while (at<rules.size()) {
            if (rules.compare(at,2,L"//")==0) { newline(); at+=2; continue; }
            if (iswspace(rules[at])) { ++at; continue; }
            std::vector<Fragment> word; float total=0;
            while (at<rules.size() && !iswspace(rules[at]) && rules.compare(at,2,L"//")!=0) {
                const auto c=rules[at++]; if (c==L'|') { bold=!bold; continue; } if (c==L'`') continue;
                Fragment part{{},{},bold,0};
                if (c==L'[' || c==L'{') {
                    const auto end=rules.find(c==L'['?L']':L'}',at);
                    if (end!=rules.npos && end-at<12) {
                        part.text=rules.substr(at,end-at); at=end+1;
                        if (!part.text.empty() && part.text.front()==L'!') part.text.erase(0,1);
                        part.symbol=c==L'{'?"vp":"coin";
                        if (c==L'[' && !part.text.empty()) {
                            const auto type=part.text.back();
                            if (type==L'D'||type==L'P'||type==L'S') { part.text.pop_back(); part.symbol=type==L'D'?"debt":type==L'P'?"potion":"sun"; }
                        }
                    } else part.text=c;
                } else {
                    part.text=c;
                    while (at<rules.size() && !iswspace(rules[at]) && rules.compare(at,2,L"//")!=0 &&
                           std::wstring_view(L"|`[{").find(rules[at])==std::wstring_view::npos) part.text+=rules[at++];
                }
                part.width=part.symbol.empty()?text_width(part.text,size,bold):size*(part.symbol=="coin"?1.32f:1.f)+2;
                total+=part.width; word.push_back(std::move(part));
            }
            if (word.empty()) continue;
            const auto space=row.parts.empty()?0.f:text_width(L" ",size,bold);
            if (!row.parts.empty() && row.width+space+total>width) newline();
            if (!row.parts.empty()) { row.parts.push_back({L" ",{},bold,space}); row.width+=space; }
            for (auto& part:word) { if (!part.symbol.empty()) row.height=(std::max)(row.height,size*1.32f+3); row.parts.push_back(std::move(part)); }
            row.width+=total;
        }
        if (!row.parts.empty()) newline();
        for (auto& line:rows) {
            if (line.parts.size()==1 && line.parts[0].text==L"---") { line.parts.clear(); line.separator=true; }
            if (line.parts.empty()) line.height=size/2;
        }
        return rows;
    }
};
ResourceImage render(Resources& store,const ResourceCard& card) {
    const std::string cache_key=std::string("card:")+card.id; if (auto cached=store.hit(cache_key)) return cached;
    require(bool(safe_load(store,"res/card_gen/shared/txt_back")) &&
            bool(safe_load(store,card.top)) && bool(safe_load(store,card.bottom)));
    const int width=card.landscape?524:344,height=card.landscape?344:524;
    store.room(std::size_t(width)*height*4);
    auto face=std::make_shared<Gdiplus::Bitmap>(width,height,PixelFormat32bppARGB);
    require(face->GetLastStatus()==Gdiplus::Ok);
    {
        Gdiplus::Graphics graphics(face.get()); graphics.Clear(Gdiplus::Color(0,0,0,0));
        Gdiplus::GraphicsPath outline;
        outline.AddArc(1,1,30,30,180,90); outline.AddArc(width-32,1,30,30,270,90);
        outline.AddArc(width-32,height-32,30,30,0,90); outline.AddArc(1,height-32,30,30,90,90); outline.CloseFigure();
        Gdiplus::SolidBrush backing(Gdiplus::Color(255,23,21,18)); graphics.FillPath(&backing,&outline);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
        Painter p{store,graphics};
        const float w=float(width),h=float(height),art_top=card.landscape?39.f:47.f,footer=h-(card.landscape?48.f:68.f);
        auto art=safe_load(store,card.art);
        const bool full=*card.value_symbol && !*card.rules;
        const float art_height=full?footer+18-art_top:card.landscape?146.f:229.f;
        const float rules_top=art?art_top+art_height:art_top;
        p.shared("txt_back",19,art_top,w-38,footer+24-art_top);
        if (art) {
            const auto scale=(std::max)((w-38)/art->GetWidth(),art_height/art->GetHeight());
            const auto source_w=(w-38)/scale,source_h=art_height/scale;
            graphics.DrawImage(art.get(),Gdiplus::RectF(19,art_top,w-38,art_height),
                (art->GetWidth()-source_w)/2,(art->GetHeight()-source_h)/2,source_w,source_h,Gdiplus::UnitPixel);
            if (!full) p.shared("text_back_lip",19,rules_top-4,w-38,9);
        }
        art.reset();
        if (card.landscape) {
            p.place(card.left,6,31,15,h-67); p.place(card.right,w-21,31,15,h-67,true);
            p.place(card.top,5,7,w-10,43); p.place(card.bottom,5,footer,w-10,40);
        } else {
            for (float x:{13.f,w-24}) { p.shared("univ_pole",x,15,10,h-38); p.shared("univ_topclip",x-3,2,18,12); p.shared("univ_baseclip",x-3,h-20,18,7); }
            p.place(card.left,8,art_top+1,22,184); p.place(card.right,w-31,art_top+1,24,184);
            p.place(card.top,5,7,w-10,57); p.place(card.bottom,5,footer,w-10,59);
        }
        if (auto emblem=safe_load(store,card.emblem)) {
            Gdiplus::ImageAttributes tint; Gdiplus::ColorMatrix matrix{};
            const float c=card.dark?1.f:.12f; matrix.m[3][3]=1; matrix.m[4][0]=matrix.m[4][1]=matrix.m[4][2]=c; matrix.m[4][4]=1;
            tint.SetColorMatrix(&matrix);
            graphics.DrawImage(emblem.get(),Gdiplus::Rect(width-48,height-(card.landscape?37:43),29,29),0,0,INT(emblem->GetWidth()),INT(emblem->GetHeight()),Gdiplus::UnitPixel,&tint);
        }
        const auto ink=card.dark?Gdiplus::Color(255,255,248,235):Gdiplus::Color(255,22,18,14);
        p.centered(card.title,w/2,card.landscape?28.f:34.f,35,w-41,ink);
        std::vector<std::pair<std::string,std::wstring>> costs;
        if (card.coins>=0 && (card.coins || (!card.potions && !card.debt))) costs.push_back({"coin",std::to_wstring(card.coins)});
        if (card.potions) costs.push_back({"potion",card.potions>1?std::to_wstring(card.potions):L""});
        if (card.debt) costs.push_back({"debt",std::to_wstring(card.debt)});
        const float size=costs.size()==1?(card.landscape?54.f:70.f):(card.landscape?41.f:43.f);
        const float cost_end=costs.size()==1?size+5:float(costs.size())*(size-5)+10,footer_start=(std::max)(57.f,cost_end);
        p.centered(card.types,(footer_start+w-52)/2,h-(card.landscape?26.f:34.f),23,w-54-footer_start,ink);
        if (*card.value_symbol) p.symbol(card.value_symbol,card.value,w/2-44,art_top+8,88);
        if (*card.rules) {
            std::vector<Painter::Row> rows; float text_height=0,font_size=25;
            for (;font_size>=10;--font_size) {
                rows=p.layout(card.rules,font_size,w-60); text_height=0; bool fits=true;
                for (const auto& row:rows) { text_height+=row.height; fits&=row.width<=w-60; }
                if (fits && text_height<=footer-rules_top-11) break;
            }
            require(font_size>=10);
            float y=(rules_top+5+footer-3-text_height)/2;
            for (const auto& row:rows) {
                float x=(w-row.width)/2;
                if (row.separator) { Gdiplus::Pen pen(Gdiplus::Color(255,69,60,53)); graphics.DrawLine(&pen,w/3,y+row.height/2,w*2/3,y+row.height/2); }
                for (const auto& part:row.parts) {
                    if (part.symbol.empty()) p.text(part.text,x,y+(row.height-font_size)*.35f,font_size,part.bold,Gdiplus::Color(255,21,19,17));
                    else { const auto d=font_size*(part.symbol=="coin"?1.32f:1.f); p.symbol(part.symbol,part.text,x,y+(row.height-d)/2,d); }
                    x+=part.width;
                }
                y+=row.height;
            }
        }
        for (std::size_t i=0;i<costs.size();++i) p.symbol(costs[i].first,costs[i].second,1+float(i)*(size-5),h-size-(card.landscape?6.f:9.f),size);
    }
    return store.put(cache_key,std::move(face));
}
}
ResourceImage game_art(std::string_view name) {
    auto& store=resources(); std::lock_guard<std::recursive_mutex> guard(store.mutex);
    std::string path(name);
    for (const auto& alias:resource_aliases) if (name==alias.name) { path=alias.path; break; }
    return safe_load(store,path);
}
ResourceImage game_card(std::string_view id) {
    auto& store=resources(); std::lock_guard<std::recursive_mutex> guard(store.mutex);
    for (const auto& card:resource_cards) if (id==card.id) {
        try { store.initialize(); return render(store,card); } catch (...) { return {}; }
    }
    return {};
}
std::size_t resource_cache_bytes() { auto& store=resources(); std::lock_guard<std::recursive_mutex> guard(store.mutex); return store.bytes; }
bool set_resource_package_for_test(const std::filesystem::path& path) { auto& store=resources(); std::lock_guard<std::recursive_mutex> guard(store.mutex); return store.open(path); }

struct CardPreviewLoader::State {
    std::mutex mutex; std::condition_variable ready; bool stop=false, pending=false, complete=false;
    std::uint64_t generation=0; std::string requested,result_id; ResourceImage result; std::thread worker;
    State():worker([this] {
        for (;;) {
            std::unique_lock<std::mutex> lock(mutex); ready.wait(lock,[this]{return stop||pending;}); if (stop) return;
            const auto id=requested; const auto serial=generation; pending=false; lock.unlock();
            auto image=game_card(id); lock.lock();
            if (!stop && serial==generation) { result_id=id; result=std::move(image); complete=true; }
        }
    }) {}
    ~State() { { std::lock_guard<std::mutex> lock(mutex); stop=true; } ready.notify_one(); worker.join(); }
};
CardPreviewLoader::CardPreviewLoader():state_(std::make_unique<State>()) {}
CardPreviewLoader::~CardPreviewLoader()=default;
void CardPreviewLoader::request(std::string id) {
    { std::lock_guard<std::mutex> lock(state_->mutex); ++state_->generation; state_->requested=std::move(id); state_->pending=true; state_->complete=false; state_->result.reset(); }
    state_->ready.notify_one();
}
bool CardPreviewLoader::take(std::string& id,ResourceImage& image) {
    std::lock_guard<std::mutex> lock(state_->mutex); if (!state_->complete) return false;
    id=state_->result_id; image=std::move(state_->result); state_->complete=false; return true;
}
}
