#pragma once
// Include after the isolated background-game fixture.
inline DWORD clear_request(const Child& child,const fs::path& dll,DWORD request) {
    const auto local=LoadLibraryExW(dll.c_str(),nullptr,DONT_RESOLVE_DLL_REFERENCES);
    require(local!=nullptr,"Cannot inspect probe DLL.");
    const auto proc=GetProcAddress(local,"RoundtripProbe");require(proc!=nullptr,"Missing probe export.");
    std::array<DWORD,512> args{};
    args[0]=reinterpret_cast<DWORD>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW"));
    args[1]=reinterpret_cast<DWORD>(proc)-reinterpret_cast<DWORD>(local);FreeLibrary(local);args[2]=request;
    require((dll.native().size()+1)*2<sizeof(args)-12,"Probe path too long.");
    std::memcpy(args.data()+3,dll.c_str(),(dll.native().size()+1)*2);
    constexpr std::array<BYTE,23> code{0x53,0x8B,0x5C,0x24,0x08,0x8D,0x4B,0x0C,0x51,0xFF,0x13,
        0x03,0x43,0x04,0xFF,0x73,0x08,0xFF,0xD0,0x5B,0xC2,0x04,0};
    return child.query(code,args,30000);
}
