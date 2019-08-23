#include <stdint.h>
#include "../Source/rmpq/file.h"

extern "C" {
#include "shine/layer3.h"
}

struct chunk_t
{
    uint32_t offset;
    uint32_t size;
};

bool ReadWaveSection( File file, uint32_t id, chunk_t& chunk )
{
    uint32_t hdr[2];

    while ( 1 )
    {
        if ( !file.read( hdr, sizeof hdr ) )
        {
            return false;
        }
        if ( hdr[0] == id )
        {
            break;
        }
        file.seek( hdr[1], SEEK_CUR );
    }

    chunk.size = hdr[1];
    chunk.offset = (uint32_t) file.tell();
    return true;
}

struct format_t
{
    uint16_t format_tag;
    uint16_t channels;
    uint32_t samples_per_sec;
    uint32_t bytes_per_sec;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

struct mmckinfo_t
{
    uint32_t ckid;
    uint32_t cksize;
    uint32_t type;
};

bool ReadWaveFile( File file, format_t& fmt, chunk_t& chunk )
{
    mmckinfo_t hdr;
    if ( !file.read( &hdr, sizeof hdr ) )
    {
        return false;
    }
    if ( hdr.ckid != 0x46464952 || hdr.type != 0x45564157 ) // 'RIFF' 'WAVE'
    {
        return false;
    }
    chunk_t chk;
    if ( !ReadWaveSection( file, 0x20746D66, chk ) ) // 'fmt '
    {
        return false;
    }
    if ( chk.size < sizeof fmt )
    {
        return false;
    }
    if ( !file.read( &fmt, sizeof fmt ) )
    {
        return false;
    }
    file.seek( chk.size - sizeof fmt, SEEK_CUR );
    return ReadWaveSection( file, 0x61746164, chunk ); // 'data'
}

inline size_t round_up( size_t size, size_t mult )
{
    size_t chk = ( size + mult - 1 ) / mult;
    return chk * mult;
}

size_t write_wave( File dst, File src )
{
    format_t fmt;
    chunk_t chunk;
    ReadWaveFile( src, fmt, chunk );

    shine_config_t cfg;
    cfg.wave.channels = fmt.channels == 1 ? PCM_MONO : PCM_STEREO;
    cfg.wave.samplerate = fmt.samples_per_sec;
    cfg.mpeg.mode = fmt.channels == 1 ? MONO : JOINT_STEREO;
    cfg.mpeg.bitr = ( fmt.samples_per_sec > 20000 && fmt.bits_per_sample == 16 ? 128 : 64 );
    cfg.mpeg.emph = NONE;
    cfg.mpeg.copyright = 0;
    cfg.mpeg.original = 1;

    auto t = shine_initialise( &cfg );
    if ( !t )
    {
        printf( "bad format\n" );
        //t = shine_initialise( &cfg );
    }
    size_t spp = (size_t) shine_samples_per_pass( t );

    std::vector<int16_t> data;
    if ( fmt.bits_per_sample == 8 )
    {
        std::vector<uint8_t> raw( chunk.size );
        src.seek( chunk.offset );
        src.read( raw.data(), chunk.size );
        data.resize( round_up( chunk.size, spp ) );
        for ( size_t i = 0; i < chunk.size; ++i )
        {
            data[i] = int16_t( ( (int) raw[i] * 65535 / 255 ) - 32768 );
        }
    }
    else
    {
        data.resize( round_up( chunk.size / 2, spp ) );
        src.seek( chunk.offset );
        src.read( data.data(), chunk.size );
    }

    size_t total = 0;
    for ( size_t pos = 0; pos < data.size(); pos += spp * fmt.channels )
    {
        int written;
        auto buffer = shine_encode_buffer_interleaved( t, data.data() + pos, &written );
        dst.write( buffer, written );
        total += written;
    }
    return total;
}
