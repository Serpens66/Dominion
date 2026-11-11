// Offline image checks: no game process, windows, input or user profile.
#include "game_resources.cpp"
#include <iostream>
#include <iomanip>
using namespace kingdom;

std::uint64_t digest(Gdiplus::Bitmap& image) {
    Gdiplus::BitmapData data{}; Gdiplus::Rect rect(0,0,INT(image.GetWidth()),INT(image.GetHeight()));
    require(image.LockBits(&rect,Gdiplus::ImageLockModeRead,PixelFormat32bppARGB,&data)==Gdiplus::Ok);
    std::array<unsigned,256> table{};
    for (unsigned i=0;i<256;++i) { unsigned c=i; for (int bit=0;bit<8;++bit) c=(c>>1)^((c&1)?0xedb88320u:0u); table[i]=c; }
    unsigned value=0xffffffffu;
    for (unsigned y=0;y<image.GetHeight();++y) {
        const auto* row=static_cast<const unsigned char*>(data.Scan0)+std::ptrdiff_t(y)*data.Stride;
        for (unsigned x=0;x<image.GetWidth()*4;++x) value=table[(value^row[x])&255]^(value>>8);
    }
    image.UnlockBits(&data); return value^0xffffffffu;
}
int wmain(int argc,wchar_t** argv) {
    try {
        require(argc==5 || argc==6);
        require(set_resource_package_for_test(argv[1]));
        std::ifstream requests(argv[2]); std::ofstream report(argv[3]); require(bool(requests)&&bool(report));
        std::filesystem::create_directories(argv[4]);
        const CLSID png{0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0,0,0xf8,0x1e,0xf3,0x2e}};
        std::string key;
        while (std::getline(requests,key)) {
            if (!key.empty()&&key.back()=='\r') key.pop_back();
            auto image=game_art(key); report<<key<<'\t';
            if (!image) { report<<"missing\n"; continue; }
            report<<image->GetWidth()<<'\t'<<image->GetHeight()<<'\t'<<std::hex<<digest(*image)<<std::dec<<'\n';
            require(resource_cache_bytes()<=budget);
        }
        if (argc==6) { require(!game_card("market")); return 0; }
        unsigned missing=0;
        for (const auto& card:resource_cards) {
            auto image=game_card(card.id);
            report<<"card:"<<card.id<<'\t'<<(image?"ok":"missing")<<'\n';
            if (!image) { ++missing; continue; }
            require(resource_cache_bytes()<=budget);
            if (std::string_view(card.id)=="market" || std::string_view(card.id)=="miser" ||
                std::string_view(card.id)=="village" || std::string_view(card.id)=="city_quarter" ||
                std::string_view(card.id)=="capitalism" || std::string_view(card.id)=="harem") {
                const auto output=std::filesystem::path(argv[4])/(std::string(card.id)+".png");
                require(image->Save(output.c_str(),&png,nullptr)==Gdiplus::Ok);
            }
        }
        // A superseded request cannot become the currently published result.
        {
            const auto cached=game_card("miser"); require(bool(cached));
            require(game_card("miser")==cached);
            CardPreviewLoader loader;
            for (int i=0;i<100;++i) loader.request("miser");
            std::string id; ResourceImage image; bool done=false;
            for (int i=0;i<200;++i) { if(loader.take(id,image)){done=true;break;} Sleep(10); }
            require(done && image==cached && id=="miser");
            const auto before=loader.counts();
            for (int i=0;i<100;++i) loader.request("miser");
            require(before==loader.counts() && before.first==1 && before.second==1);
            loader.request("not-a-card"); done=false;
            for(int i=0;i<200;++i) { if(loader.take(id,image)){done=true;break;} Sleep(10); }
            require(done && !image && id=="not-a-card");
        }
        {
            CardPreviewLoader loader;
            for (int i=0;i<30;++i) loader.request(i%2?"market":"village");
            std::string id; ResourceImage image; bool done=false;
            for (int i=0;i<200;++i) { if (loader.take(id,image)) { done=true; break; } Sleep(10); }
            require(done && id=="market" && bool(image));
            loader.request("miser"); // Destruction while work is pending must join safely.
        }
        std::cout<<"Cards missing: "<<missing<<"; cache bytes: "<<resource_cache_bytes()<<"\n";
        return missing?1:0;
    } catch (const std::exception& error) { std::cerr<<error.what()<<"\n"; return 1; }
}
