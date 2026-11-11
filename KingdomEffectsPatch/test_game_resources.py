"""Compare native Xpack/QOI/sprite decoding against Pillow; no game process."""
import json, re, subprocess, sys, zlib, struct
from functools import lru_cache
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"SetGenerator"))
from generator_catalog import read_index, read_resource
from extract_card_images import decode_artwork, decode_texture

def main():
    root=Path(__file__).resolve().parent
    build=root/"out"/"staging"
    output=build/"resource-check"
    output.mkdir(exist_ok=True)
    xpack=Path(sys.argv[1])
    metadata=(build/"resource_catalog.generated.hpp").read_text(encoding="utf-8")
    paths=sorted(set(re.findall(r'"(res/[^"\n]+)"',metadata)))
    request=output/"requests.txt"; report=output/"native.tsv"
    request.write_text("\n".join(paths)+"\n",encoding="ascii")
    result=subprocess.run([str(build/"resources_test.exe"),str(xpack),str(request),str(report),str(output)],check=False)
    native={row[0]:row[1:] for line in report.read_text().splitlines() if (row:=line.split("\t"))}
    failed=[]; compared=0; missing=[]
    with xpack.open("rb") as package:
        index=read_index(package)
        def read(key): return read_resource(package,index,key)
        @lru_cache(maxsize=8)
        def atlas(key): return decode_texture(read(key))
        for path in paths:
            try: image=decode_artwork(read,atlas,path).convert("RGBA")
            except (KeyError,ValueError,OSError,struct.error):
                if native[path]!=["missing"]: failed.append((path,"expected missing",native[path]))
                missing.append(path);continue
            expected=[str(image.width),str(image.height),format(zlib.crc32(image.tobytes("raw","BGRA")),"x")]
            if native[path]!=expected: failed.append((path,expected,native[path]))
            compared+=1
    failed_cards=[key for key,val in native.items() if key.startswith("card:") and val!=["ok"]]
    print("Pixel comparisons:",compared,"mismatches:",len(failed),"missing cards:",failed_cards)
    for failure in failed[:12]: print(failure)
    # Invalid index, bounds, dimensions, truncation, atlas cycles, and QOI operations.
    def pack(name,data):
        entry=bytearray(168);struct.pack_into("<II",entry,12,180,len(data));entry[40:40+len(name)]=name.encode()
        return b"KCPX"+struct.pack("<II",7,1)+entry+data
    def texture(qoi):
        header=bytearray(136);struct.pack_into("<Q",header,128,136);return header+qoi
    end=b"\0"*7+b"\1"
    qoi=b"qoif"+struct.pack(">IIBB",1,1,4,0)+b"\xff\x12\x34\x56\x78"+end
    cases=[("valid",pack("fixture",texture(qoi)),True),
        ("truncated",pack("fixture",texture(qoi[:-9])),False),
        ("dimensions",pack("fixture",texture(b"qoif"+struct.pack(">IIBB",65535,65535,4,0)+end)),False)]
    sprite=bytearray(150);struct.pack_into("<II",sprite,40,1,1);sprite[120:128]=b"fixture\0"
    cases.append(("cycle",pack("fixture",sprite),False))
    invalid=bytearray(pack("fixture",texture(qoi)));struct.pack_into("<I",invalid,24,0xffffffff)
    bad=output/"invalid-index.xpack";bad.write_bytes(invalid)
    fixture_request=output/"fixture.txt";fixture_request.write_text("fixture\n")
    assert subprocess.run([str(build/"resources_test.exe"),str(bad),str(fixture_request),str(output/"invalid.tsv"),str(output),"--decode-only"],capture_output=True).returncode!=0
    for name,data,valid in cases:
        fixture=output/(name+".xpack");fixture.write_bytes(data)
        fixture_report=output/(name+".tsv")
        run=subprocess.run([str(build/"resources_test.exe"),str(fixture),str(fixture_request),str(fixture_report),str(output),"--decode-only"],capture_output=True)
        assert run.returncode==0,(name,run.stdout,run.stderr)
        fields=fixture_report.read_text().strip().split("\t")
        assert (fields[1]!="missing")==valid,(name,fields)
        if valid: assert fields[1:]==["1","1",format(zlib.crc32(bytes([0x12,0x34,0x56,0x78])),"x")],fields
    assert not failed and not failed_cards and result.returncode==0
    assert not missing,missing
    print("PASS: native pixels, all card faces, cache limit, latest-request handling, close during loading and malformed resources.")

if __name__=="__main__": main()
