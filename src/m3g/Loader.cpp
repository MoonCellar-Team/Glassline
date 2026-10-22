#include "m3g/Loader.h"
#include "m3g/GZipStreamBasic.h"
#include "m3g/OpaqueObject.h"
#include "runtime/midp/DataInputStream.h"
#include "runtime/util/VectorSE.h"

namespace m3g {
bool loadSection(midp::DataInputStream&,util::VectorSE<midp::Object>*,util::VectorSE<midp::Object>*);
const unsigned char Loader::M3G_FILE_IDENTIFIER[12]={0xab,0x4a,0x53,0x52,0x31,0x38,0x34,0xbb,0x0d,0x0a,0x1a,0x0a};

midp::array<Object3D*> Loader::load(midp::InputStream* input) {
    if(!input) return midp::array<Object3D*>();
    OpaqueObject<midp::DataInputStream,16> dataStorage(input);
    midp::DataInputStream* data=dataStorage.get();
    data->mark(-1);
    signed char header[12]; data->readFully(header,12);
    bool compressed=false;
    for(int i=0;i<12;++i) if(static_cast<unsigned char>(header[i])!=M3G_FILE_IDENTIFIER[i]) { compressed=true; break; }
    GZipStreamBasic gzip;
    if(compressed) {
        data->reset();
        if(!gzip.gzopen(data)) return midp::array<Object3D*>();
        gzip.read(header,12);
        for(int i=0;i<12;++i) if(static_cast<unsigned char>(header[i])!=M3G_FILE_IDENTIFIER[i]) return midp::array<Object3D*>();
    }
    OpaqueObject<util::VectorSE<midp::Object>,24> rootStorage(30,30),objectStorage(30,30);
    auto* roots=rootStorage.get(); auto* objects=objectStorage.get();
    objects->addElement(nullptr);
    if(compressed) {
        midp::INCREF(&gzip);
        {
            OpaqueObject<midp::DataInputStream,16> uncompressedStorage(&gzip);
            while(loadSection(*uncompressedStorage.get(),objects,roots)) {}
        }
        midp::DECREF_NO_DELETE(&gzip); gzip.gzclose();
    } else while(loadSection(*data,objects,roots)) {}
    data->close();
    midp::array<Object3D*> result(roots->size());
    for(int i=0;i<roots->size();++i) {
        midp::Object* object=roots->elementAt(i); midp::INCREF(object); result[i]=static_cast<Object3D*>(object);
    }
    return result;
}
}
