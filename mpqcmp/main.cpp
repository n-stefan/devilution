#include <stdint.h>
#include <algorithm>
#include <set>
#include <string>
typedef unsigned long DWORD;
typedef void* HWND;
typedef int BOOL;
#include "../Source/appfat.h"
#include "../Source/rmpq/file.h"
#include "../Source/rmpq/archive.h"
#include "../Source/rmpq/common.h"

#ifdef EMSCRIPTEN

#include <emscripten.h>

EM_JS(void, get_file_contents, (void* ptr, size_t offset, size_t size), {
  self.DApi.get_file_contents(HEAPU8.subarray(ptr, ptr + size), offset);
});

EM_JS(void, put_file_size, (size_t size), {
  self.DApi.put_file_size(size);
});

EM_JS(void, put_file_contents, (void* ptr, size_t offset, size_t size), {
  self.DApi.put_file_contents(HEAPU8.subarray(ptr, ptr + size), offset);
});

#endif

class ChunkedBuffer : public FileBuffer
{
public:
    ChunkedBuffer()
    {
        chunks_.reserve( 256 );
    }

    int getc() override
    {
        if ( pos_ < size_ )
        {
            auto result = chunks_[pos_ / CHUNK_SIZE][pos_ % CHUNK_SIZE];
            ++pos_;
            return result;
        }
        return EOF;
    }
    void putc( int chr ) override
    {
        size_t chk = pos_ / CHUNK_SIZE;
        if ( chk >= chunks_.size() )
        {
            chunks_.resize( chk + 1 );
        }
        chunks_[chk][pos_ % CHUNK_SIZE] = chr;
        ++pos_;
        if ( pos_ > size_ )
        {
            size_ = pos_;
        }
    }

    uint64_t tell() const override
    {
        return pos_;
    }
    void seek( int64_t pos, int mode ) override
    {
        switch ( mode )
        {
        case SEEK_CUR:
            pos += pos_;
            break;
        case SEEK_END:
            pos += size_;
            break;
        }
        if ( pos < 0 ) pos = 0;
        pos_ = (size_t) pos;
    }
    uint64_t size() override
    {
        return size_;
    }

    size_t read( void* ptr, size_t size ) override
    {
        uint8_t* dst = (uint8_t*) ptr;
        size_t done = 0;
        while ( size && pos_ < size_ )
        {
            size_t chk = pos_ / CHUNK_SIZE;
            size_t offs = pos_ % CHUNK_SIZE;
            size_t have = std::min( size, std::min( size_ - pos_, CHUNK_SIZE - offs ) );
            memcpy( dst, chunks_[chk].get() + offs, have );
            pos_ += have;
            done += have;
            dst += have;
            size -= have;
        }
        return done;
    }

    size_t write( void const* ptr, size_t size ) override
    {
        if ( pos_ + size > size_ )
        {
            size_ = pos_ + size;
            size_t chks = ( size_ + CHUNK_SIZE - 1 ) / CHUNK_SIZE;
            if ( chks > chunks_.size() )
            {
                chunks_.resize( chks );
            }
        }
        size_t done = 0;
        const uint8_t* src = (uint8_t*) ptr;
        while ( size )
        {
            size_t chk = pos_ / CHUNK_SIZE;
            size_t offs = pos_ % CHUNK_SIZE;
            size_t have = std::min( size, CHUNK_SIZE - offs );
            memcpy( chunks_[chk].get() + offs, src, have );
            pos_ += have;
            done += have;
            src += have;
            size -= have;
        }
        return done;
    }

    void truncate() override
    {
        size_ = pos_;
        size_t chks = ( size_ + CHUNK_SIZE - 1 ) / CHUNK_SIZE;
        if ( chks > chunks_.size() )
        {
            chunks_.resize( chks );
        }
    }

    void flush()
    {
#ifdef EMSCRIPTEN
        put_file_size( size_ );
        size_t remain = size_;
        for ( size_t i = 0; i < chunks_.size(); ++i )
        {
            put_file_contents( chunks_[i].get(), i * CHUNK_SIZE, std::min( remain, CHUNK_SIZE ) );
            remain -= CHUNK_SIZE;
        }
#else
        File out( "output.mpq", "wb" );
        size_t remain = size_;
        for ( size_t i = 0; i < chunks_.size(); ++i )
        {
            out.write( chunks_[i].get(), std::min( remain, CHUNK_SIZE ) );
            remain -= CHUNK_SIZE;
        }
#endif
    }

private:
    static const size_t CHUNK_SIZE = 1048576;
    class chunk_t : public std::unique_ptr<uint8_t[]>
    {
    public:
        chunk_t()
            : std::unique_ptr<uint8_t[]>( std::make_unique<uint8_t[]>( CHUNK_SIZE ) )
        {
            memset( get(), 0, CHUNK_SIZE );
        }
    };
    std::vector<chunk_t> chunks_;
    size_t pos_ = 0;
    size_t size_ = 0;
};

#ifndef EMSCRIPTEN

#include <io.h>

class StdFileBuffer : public FileBuffer
{
    FILE* file_;
public:
    StdFileBuffer( FILE* file )
        : file_( file )
    {
    }
    ~StdFileBuffer()
    {
        fclose( file_ );
    }

    int getc()
    {
        return fgetc( file_ );
    }
    void putc( int chr )
    {
        fputc( chr, file_ );
    }

    uint64_t tell() const
    {
        return ftell( file_ );
    }
    void seek( int64_t pos, int mode )
    {
        fseek( file_, (long) pos, mode );
    }

    size_t read( void* ptr, size_t size )
    {
        return fread( ptr, 1, size, file_ );
    }
    size_t write( void const* ptr, size_t size )
    {
        return fwrite( ptr, 1, size, file_ );
    }

    void truncate()
    {
        _chsize( _fileno( file_ ), (long) ftell( file_ ) );
    }
};

File::File( const char* name, const char* mode )
{
    FILE* file = fopen( name, mode );
    if ( file )
    {
        file_ = std::make_shared<StdFileBuffer>( file );
    }
}

#else

class ReadOnlyFile : public FileBuffer {
  size_t pos_ = 0;
  std::vector<uint8_t> data_;
  size_t size_;
  size_t loadPos_ = 0;
public:
  static const size_t MAX_CHUNK = 1 << 21;

  ReadOnlyFile(size_t size)
    : size_(size) {
  }

  int getc() {
    if (pos_ >= size_) {
      return EOF;
    }
    if (pos_ >= loadPos_ && pos_ < loadPos_ + data_.size()) {
      return data_[pos_++ - loadPos_];
    }
    loadPos_ = pos_;
    data_.resize(std::min(MAX_CHUNK, size_ - pos_));
    get_file_contents(data_.data(), loadPos_, data_.size());
    return data_[pos_++ - loadPos_];
  }
  void putc(int chr) {
  }

  uint64_t tell() const {
    return pos_;
  }
  void seek(int64_t pos, int mode) {
    switch (mode) {
    case SEEK_CUR:
      pos += pos_;
      break;
    case SEEK_END:
      pos += size_;
      break;
    }
    if (pos < 0) pos = 0;
    //if (static_cast<size_t>(pos) > data_.size()) pos = data_.size();
    pos_ = (size_t) pos;
  }
  uint64_t size() {
    return size_;
  }

  size_t read(void* ptr, size_t size) {
    if (pos_ >= size_) {
      return 0;
    }
    if (size + pos_ > size_) {
      size = size_ - pos_;
    }
    if (pos_ >= loadPos_ && pos_ + size <= loadPos_ + data_.size()) {
      memcpy(ptr, data_.data() + pos_ - loadPos_, size);
      pos_ += size;
      return size;
    }
    if (size <= MAX_CHUNK) {
      loadPos_ = pos_;
      data_.resize(std::min(MAX_CHUNK, size_ - pos_));
      get_file_contents(data_.data(), loadPos_, data_.size());
      memcpy(ptr, data_.data(), size);
      pos_ += size;
      return size;
    }
    get_file_contents(ptr, pos_, size);
    pos_ += size;
    return size;
  }

  size_t write(void const* ptr, size_t size) {
    return 0;
  }

  void truncate() {}
};


#endif

#ifdef EMSCRIPTEN
EM_JS(void, do_error, (const char* err), {
  var end = HEAPU8.indexOf( 0, err );
  var text = String.fromCharCode.apply(null, HEAPU8.subarray( err, end ));
  self.DApi.exit_error( text );
});
EM_JS(void, do_progress, (int done, int total), {
  self.DApi.progress(done, total);
});
#else
void do_error( const char* err )
{
    printf( "%s\r\n", err );
    exit( 0 );
}
void do_progress( int done, int total )
{
    printf( "\r%d / %d", done, total );
}
#endif

void ErrMsg( const char* text, const char *log_file_path, int log_line_nr )
{
    do_error( text );
}

const size_t BLOCK_TYPE = 7;
const size_t BLOCK_SIZE = 1 << ( 9 + BLOCK_TYPE );

size_t write_wave( File dst, File src );

class MpqWriter
{
public:
    MpqWriter( size_t tableSize )
        : buffer_( std::make_shared<ChunkedBuffer>() )
        , file_( buffer_ )
    {
        header_.id = mpq::MPQHeader::signature;
        header_.headerSize = mpq::MPQHeader::size_v1;
        header_.archiveSize = 0;
        header_.formatVersion = 1;
        header_.blockSize = (uint16_t) BLOCK_TYPE;
        header_.hashTablePos = header_.headerSize;
        header_.blockTablePos = header_.hashTablePos + tableSize * sizeof( mpq::MPQHashEntry );
        header_.hashTableSize = tableSize;
        header_.blockTableSize = tableSize;
        hashTable_.resize( tableSize );
        blockTable_.resize( tableSize );
        memset( blockTable_.data(), 0, sizeof( mpq::MPQBlockEntry ) * tableSize );
        memset( hashTable_.data(), -1, sizeof( mpq::MPQHashEntry ) * tableSize );
        file_.seek( header_.blockTablePos + tableSize * sizeof( mpq::MPQBlockEntry ) );
    }

    mpq::MPQBlockEntry& alloc_file( const char* name )
    {
        size_t index = mpq::hashString( name, mpq::HASH_OFFSET ) % hashTable_.size();
        while ( hashTable_[index].blockIndex != mpq::MPQHashEntry::EMPTY && hashTable_[index].blockIndex != mpq::MPQHashEntry::DELETED )
        {
            index = ( index + 1 ) % hashTable_.size();
        }

        hashTable_[index].name1 = mpq::hashString( name, mpq::HASH_NAME1 );
        hashTable_[index].name2 = mpq::hashString( name, mpq::HASH_NAME2 );
        hashTable_[index].locale = 0;
        hashTable_[index].platform = 0;
        hashTable_[index].blockIndex = blockPos_;

        return blockTable_[blockPos_++];
    }

    void write_file( const char* name, File file )
    {
        auto& block = alloc_file( name );
        if ( !strcmp( mpq::path_ext( name ), ".wav" ) && file.size() >= 4096 )
        {
            write_wav( file, block );
        }
        else
        {
            write_mpq( file, block );
        }
    }

    void flush()
    {
        mpq::encryptBlock( hashTable_.data(), hashTable_.size() * sizeof( mpq::MPQHashEntry ), mpq::hashString( "(hash table)", mpq::HASH_KEY ) );
        mpq::encryptBlock( blockTable_.data(), blockTable_.size() * sizeof( mpq::MPQBlockEntry ), mpq::hashString( "(block table)", mpq::HASH_KEY ) );

        header_.archiveSize = (uint32_t) file_.size();
        file_.seek( 0 );
        file_.write( &header_, header_.headerSize );
        file_.write( hashTable_.data(), hashTable_.size() * sizeof( mpq::MPQHashEntry ) );
        file_.write( blockTable_.data(), blockTable_.size() * sizeof( mpq::MPQBlockEntry ) );
        file_.seek( 0 );

        buffer_->flush();
    }

private:
    std::shared_ptr<ChunkedBuffer> buffer_;
    File file_;
    mpq::MPQHeader header_;
    std::vector<mpq::MPQHashEntry> hashTable_;
    std::vector<mpq::MPQBlockEntry> blockTable_;
    size_t blockPos_ = 0;

    void write_mpq( File src, mpq::MPQBlockEntry& entry )
    {
        uint8_t BLOCK_BUF[BLOCK_SIZE];
        uint8_t CMP_BUF[BLOCK_SIZE + 1024];

        entry.filePos = (uint32_t) file_.tell();
        entry.flags = mpq::FileFlags::CompressMulti | mpq::FileFlags::Exists;
        entry.fSize = (uint32_t) src.size();
        size_t num_blocks = ( entry.fSize + BLOCK_SIZE - 1 ) / BLOCK_SIZE;
        entry.cSize = ( num_blocks + 1 ) * 4;
        std::vector<uint32_t> blockPos( num_blocks + 1 );
        file_.seek( entry.cSize, SEEK_CUR );
        blockPos[0] = entry.cSize;
        for ( size_t i = 0; i < num_blocks; ++i )
        {
            uint32_t count = (uint32_t) src.read( BLOCK_BUF, BLOCK_SIZE );
            uint32_t comps = sizeof CMP_BUF;
            if ( !mpq::multi_compress( BLOCK_BUF, count, CMP_BUF, &comps, 0x02, nullptr ) )
            {
                ERROR_MSG( "compression error" );
            }
            //if ( !mpq::multi_decompress( CMP_BUF, comps, BLOCK_BUF, &count, nullptr ) )
            //{
            //    ERROR_MSG( "compression error" );
            //}
            file_.write( CMP_BUF, comps );
            blockPos[i + 1] = blockPos[i] + comps;
            entry.cSize += comps;
        }
        file_.seek( entry.filePos, SEEK_SET );
        file_.write( blockPos.data(), blockPos.size() * 4 );
        file_.seek( entry.filePos + entry.cSize, SEEK_SET );
    }
    void write_wav( File src, mpq::MPQBlockEntry& entry )
    {
        entry.filePos = (uint32_t) file_.tell();
        entry.flags = mpq::FileFlags::Exists;
        entry.fSize = (uint32_t) write_wave( file_, src );
        entry.cSize = entry.fSize;
    }
};

const int NumFiles = 2908;
extern const char* ListFile[NumFiles];

#ifdef EMSCRIPTEN
EMSCRIPTEN_KEEPALIVE
extern "C" void DApi_MpqCmp( size_t size )
{
    File source( std::make_shared<ReadOnlyFile>( size ) );
#else
int main()
{
    File source( "C:\\Work\\mpq\\DIABDAT0.MPQ", "rb" );
#endif
    mpq::Archive arc( source );
    MpqWriter writer( arc.getMaxFiles() );
    int count = 0;
    for ( auto line : ListFile )
    {
        do_progress( ++count, NumFiles );
        auto file = arc.load( line );
        if ( file )
        {
            writer.write_file( line, file );
        }
    }
    writer.flush();
}
