#!/usr/bin/env python3
"""Compile the reconstructed AS3 with Apache Flex, then restore its SWF assets.

This is a real high-level compilation. No original ABC or P-code method bodies
are placed in the source-built output. Original graphics/timelines are retained
from the authoritative vanilla interface archive.
"""
import hashlib
import json
import shutil
from pathlib import Path
import struct
import subprocess
import zlib

ROOT=Path(__file__).resolve().parent

def unpack(data):
    if data[:3]==b'CWS': raw=zlib.decompress(data[8:])
    elif data[:3]==b'FWS': raw=data[8:]
    else: raise ValueError('Only FWS/CWS are supported')
    assert len(raw)+8==struct.unpack_from('<I',data,4)[0]
    offset=(5+4*(raw[0]>>3)+7)//8+4
    prefix=raw[:offset]; tags=[]
    while offset<len(raw):
        start=offset
        header,=struct.unpack_from('<H',raw,offset); offset+=2
        size=header&63
        if size==63: size,=struct.unpack_from('<I',raw,offset); offset+=4
        payload=raw[offset:offset+size]; offset+=size
        if len(payload)!=size: raise ValueError('Truncated tag')
        tags.append((header>>6,payload,raw[start:offset]))
    return prefix,tags

def abc_payload(tags):
    matches=[payload for code,payload,_ in tags if code==82]
    if len(matches)!=1: raise ValueError('Expected exactly one DoABC2 tag')
    payload=matches[0]; end=payload.index(b'\0',4)+1
    return payload[:end],payload[end:]

def graft(baseline,compiled):
    prefix,tags=unpack(baseline)
    _,code_tags=unpack(compiled)
    header,_=abc_payload(tags)
    _,new_abc=abc_payload(code_tags)
    payload=header+new_abc
    raw=prefix+b''.join(struct.pack('<HI',(82<<6)|63,len(payload))+payload if code==82 else encoded for code,_,encoded in tags)
    output=b'CWS'+baseline[3:4]+struct.pack('<I',8+len(raw))+zlib.compress(raw,6)
    check_prefix,check_tags=unpack(output)
    assert prefix==check_prefix
    assert [b for code,_,b in tags if code!=82]==[b for code,_,b in check_tags if code!=82]
    assert abc_payload(check_tags)[1]==new_abc
    return output,new_abc,len(tags)-1

def main():
    output=ROOT/'Built'; output.mkdir(exist_ok=True)
    # The vanilla archive is the authoritative asset baseline, hash-checked below.
    source=ROOT/'Source/reconstructed-as'
    classes=sorted('.'.join(p.relative_to(source).with_suffix('').parts) for p in source.rglob('*.as'))
    sdk=ROOT/'tools/apache-flex'
    mxmlc=sdk/'lib/mxmlc.jar'
    frameworks=sdk/'frameworks'
    playerglobal=ROOT/'tools/flashlib/playerglobal32_0.swc'
    java=shutil.which('java')
    missing=[]
    if java is None: missing.append('java (PATH)')
    if not mxmlc.is_file(): missing.append(str(mxmlc.relative_to(ROOT)))
    if not frameworks.is_dir(): missing.append(str(frameworks.relative_to(ROOT)) + '/')
    if not playerglobal.is_file(): missing.append(str(playerglobal.relative_to(ROOT)))
    if missing:
        raise SystemExit('Missing SWF build requirement(s):\n  ' + '\n  '.join(missing))
    compiled=output/'Pipboy_StatsPage-source-code-only.swf'
    command=[java,'-XX:-UsePerfData','-Djava.awt.headless=true','-Xmx1g','-jar',str(mxmlc),
             '+flexlib='+str(frameworks),'-theme=','-locale=','-library-path=','-load-config=',
             '-external-library-path='+str(playerglobal),
             '-source-path='+str(source),'-includes='+','.join(classes),
             '-debug=false','-optimize=true','-strict=true','-as3=true','-omit-trace-statements=false',
             '-target-player=11.2','-swf-version=15','-default-size=876,700','-default-frame-rate=30',
             '-output='+str(compiled),str(source/'Pipboy_StatsPage.as')]
    (output/'source-compiler-command.json').write_text(json.dumps(command,indent=2)+'\n')
    result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True)
    (output/'source-compiler.log').write_text(result.stdout+result.stderr)
    print(result.stdout+result.stderr,end='')
    result.check_returncode()
    baseline=(ROOT/'Reference/Pipboy_StatsPage-vanilla-reference.swf').read_bytes()
    assert hashlib.sha256(baseline).hexdigest()=='ce93c9a7ac8f1ec29bfabb837df6ddefed0ace1c124cd3a8b5f09b7fa39652d8'
    data,abc,tag_count=graft(baseline,compiled.read_bytes())
    final=output/'Pipboy_StatsPage.swf'; final.write_bytes(data)
    report=dict(file=final.name,compiler='Apache Flex 4.16.1 / ASC 1, build 20171115',
                classes_included=len(classes),compiler_exit_code=result.returncode,
                asset_baseline='Reference/Pipboy_StatsPage-vanilla-reference.swf',
                asset_baseline_sha256=hashlib.sha256(baseline).hexdigest(),
                size_bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),
                compiled_abc_sha256=hashlib.sha256(abc).hexdigest(),
                non_abc_tags_preserved_byte_for_byte=tag_count,
                compiled_abc_preserved_byte_for_byte=True,
                original_method_bodies_used_in_source_output=False,
                byte_identical_to_original=data==baseline,game_tested=False,
                api_library='playerglobal32_0.swc; output targets Flash 11.2 / SWF 15')
    (output/'source-verification.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__': main()
