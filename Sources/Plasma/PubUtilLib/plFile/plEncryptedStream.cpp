/*==LICENSE==*

CyanWorlds.com Engine - MMOG client, server and tools
Copyright (C) 2011  Cyan Worlds, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Additional permissions under GNU GPL version 3 section 7

If you modify this Program, or any covered work, by linking or
combining it with any of RAD Game Tools Bink SDK, Autodesk 3ds Max SDK,
NVIDIA PhysX SDK, Microsoft DirectX SDK, OpenSSL library, Independent
JPEG Group JPEG library, Microsoft Windows Media SDK, or Apple QuickTime SDK
(or a modified version of those libraries),
containing parts covered by the terms of the Bink SDK EULA, 3ds Max EULA,
PhysX SDK EULA, DirectX SDK EULA, OpenSSL and SSLeay licenses, IJG
JPEG Library README, Windows Media SDK EULA, or QuickTime SDK EULA, the
licensors of this Program grant you additional
permission to convey the resulting work. Corresponding Source for a
non-source form of such a combination shall include the source code for
the parts of OpenSSL and IJG JPEG Library used as well as that of the covered
work.

You can contact Cyan Worlds, Inc. by email legal@cyan.com
 or by snail mail at:
      Cyan Worlds, Inc.
      14617 N Newport Hwy
      Mead, WA   99021

*==LICENSE==*/
#include "plEncryptedStream.h"

#include "plFileUtils.h"
#include "hsSTLStream.h"

#include <time.h>

static const uint32_t kDefaultKey[4] = { 0x6c0a5452, 0x3827d0f, 0x3a170b92, 0x16db7fc2 };
static const int kEncryptChunkSize = 8;

static const char* kOldMagicString = "BriceIsSmart";
static const char* kMagicString    = "whatdoyousee";
static const int kMagicStringLen = 12;

static const int kFileStartOffset = kMagicStringLen + sizeof(uint32_t);

static const int kMaxBufferedFileSize = 10*1024;

plEncryptedStream::plEncryptedStream(uint32_t* key) :
    fRef(nil),
    fActualFileSize(0),
    fBufferedStream(false),
    fRAMStream(nil),
    fWriteFileName(nil),
    fOpenMode(kOpenFail)
{
    if (key)
        memcpy(&fKey, key, sizeof(kDefaultKey));
    else
        memcpy(&fKey, &kDefaultKey, sizeof(kDefaultKey));
}

plEncryptedStream::~plEncryptedStream()
{
}

//
// Tiny Encryption Algorithm
// http://vader.brad.ac.uk/tea/tea.shtml
//
// A potential weakness in this implementation is the fact that a known value
// (length of the original file) is written at the start of the encrypted file.  -Colin
//
// Oh, and also there's some kind of potential weakness in TEA that they fixed with XTEA,
// but frankly, who cares.  No one is going to break the encryption, they'll just get the
// key out of the exe or memory.
//
void plEncryptedStream::IEncipher(uint32_t* const v)
{
    register uint32_t y=v[0], z=v[1], sum=0, delta=0x9E3779B9, n=32;

    while (n-- > 0)
    {
        y += (z << 4 ^ z >> 5) + z ^ sum + fKey[sum&3];
        sum += delta;
        z += (y << 4 ^ y >> 5) + y ^ sum + fKey[sum>>11 & 3];
    }

   v[0]=y; v[1]=z;
}

void plEncryptedStream::IDecipher(uint32_t* const v)
{
    register uint32_t y=v[0], z=v[1], sum=0xC6EF3720, delta=0x9E3779B9, n=32;

    // sum = delta<<5, in general sum = delta * n

    while (n-- > 0)
    {
        z -= (y << 4 ^ y >> 5) + y ^ sum + fKey[sum>>11 & 3];
        sum -= delta;
        y -= (z << 4 ^ z >> 5) + z ^ sum + fKey[sum&3];
    }
   
    v[0]=y; v[1]=z;
}

hsBool plEncryptedStream::Open(const char* name, const char* mode)
{
    wchar_t* wName = hsStringToWString(name);
    wchar_t* wMode = hsStringToWString(mode);
    hsBool ret = Open(wName, wMode);
    delete [] wName;
    delete [] wMode;
    return ret;
}

hsBool plEncryptedStream::Open(const wchar_t* name, const wchar_t* mode)
{
    if (wcscmp(mode, L"rb") == 0)
    {
        fRef = hsWFopen(name, mode);
        fPosition = 0;

        if (!fRef)
            return false;

        // Make sure our special magic string is there
        if (!ICheckMagicString(fRef))
        {
            fclose(fRef);
            return false;
        }

        fread(&fActualFileSize, sizeof(uint32_t), 1, fRef);

        // The encrypted stream is inefficient if you do reads smaller than
        // 8 bytes.  Since we do a lot of those, any file under a size threshold
        // is buffered in memory
        if (fActualFileSize <= kMaxBufferedFileSize)
            IBufferFile();

        fOpenMode = kOpenRead;

        return true;
    }
    else if (wcscmp(mode, L"wb") == 0)
    {
        fRAMStream = new hsVectorStream;
        fWriteFileName = new wchar_t[wcslen(name) + 1];
        wcscpy(fWriteFileName, name);
        fPosition = 0;

        fOpenMode = kOpenWrite;
        fBufferedStream = true;

        return true;
    }
    else
    {
        hsAssert(0, "Unsupported open mode");
        fOpenMode = kOpenFail;
        return false;
    }
}

hsBool plEncryptedStream::Close()
{
    int rtn = false;

    if (fOpenMode == kOpenWrite)
    {
        fRAMStream->Rewind();
        rtn = IWriteEncypted(fRAMStream, fWriteFileName);
    }
    if (fRef)
    {
        rtn = (fclose(fRef) == 0);
        fRef = nil;
    }

    if (fRAMStream)
    {
        delete fRAMStream;
        fRAMStream = nil;
    }

    if (fWriteFileName)
    {
        delete [] fWriteFileName;
        fWriteFileName = nil;
    }

    fActualFileSize = 0;
    fBufferedStream = false;
    fOpenMode = kOpenFail;

    return rtn;
}

uint32_t plEncryptedStream::IRead(uint32_t bytes, void* buffer)
{
    if (!fRef)
        return 0;
    int numItems = (int)(::fread(buffer, 1 /*size*/, bytes /*count*/, fRef));
    fBytesRead += numItems;
    fPosition += numItems;
    if ((unsigned)numItems < bytes) {
        if (feof(fRef)) {
            // EOF ocurred
            char str[128];
            sprintf(str, "Hit EOF on UNIX Read, only read %d out of requested %d bytes\n", numItems, bytes);
            hsDebugMessage(str, 0);
        }
        else {
            hsDebugMessage("Error on UNIX Read", ferror(fRef));
        }
    }
    return numItems;
}

void plEncryptedStream::IBufferFile()
{
    fRAMStream = new hsVectorStream;
    char buf[1024];
    while (!AtEnd())
    {
        uint32_t numRead = Read(1024, buf);
        fRAMStream->Write(numRead, buf);
    }
    fRAMStream->Rewind();

    fBufferedStream = true;
    fclose(fRef);
    fRef = nil;
    fPosition = 0;
}

hsBool plEncryptedStream::AtEnd()
{
    if (fBufferedStream)
        return fRAMStream->AtEnd();
    else
        return (GetPosition() == fActualFileSize);
}

void plEncryptedStream::Skip(uint32_t delta)
{
    if (fBufferedStream)
    {
        fRAMStream->Skip(delta);
        fPosition = fRAMStream->GetPosition();
    }
    else if (fRef)
    {
        fBytesRead += delta;
        fPosition += delta;
        fseek(fRef, delta, SEEK_CUR);
    }
}

void plEncryptedStream::Rewind()
{
    if (fBufferedStream)
    {
        fRAMStream->Rewind();
        fPosition = fRAMStream->GetPosition();
    }
    else if (fRef)
    {
        fBytesRead = 0;
        fPosition = 0;
        fseek(fRef, kFileStartOffset, SEEK_SET);
    }
}

void plEncryptedStream::FastFwd()
{
    if (fBufferedStream)
    {
        fRAMStream->FastFwd();
        fPosition = fRAMStream->GetPosition();
    }
    else if (fRef)
    {
        fseek(fRef, kFileStartOffset+fActualFileSize, SEEK_SET);
        fBytesRead = fPosition = ftell(fRef);
    }
}

uint32_t plEncryptedStream::GetEOF()
{
    return fActualFileSize;
}

uint32_t plEncryptedStream::Read(uint32_t bytes, void* buffer)
{
    if (fBufferedStream)
    {
        uint32_t numRead = fRAMStream->Read(bytes, buffer);
        fPosition = fRAMStream->GetPosition();
        return numRead;
    }

    uint32_t startPos = fPosition;

    // Offset into the first buffer (0 if we are aligned on a chunk, which means no extra block read)
    uint32_t startChunkPos = startPos % kEncryptChunkSize;
    // Amount of data in the partial first chunk (0 if we're aligned)
    uint32_t startAmt = (startChunkPos != 0) ? hsMinimum(kEncryptChunkSize - startChunkPos, bytes) : 0;

    uint32_t totalNumRead = IRead(bytes, buffer);

    uint32_t numMidChunks = (totalNumRead - startAmt) / kEncryptChunkSize;
    uint32_t endAmt = (totalNumRead - startAmt) % kEncryptChunkSize;

    // If the start position is in the middle of a chunk we need to rewind and
    // read that whole chunk in and decrypt it.
    if (startChunkPos != 0)
    {
        // Move to the start of this chunk
        SetPosition(startPos-startChunkPos);

        // Read in the chunk and decrypt it
        char buf[kEncryptChunkSize];
        uint32_t numRead = IRead(kEncryptChunkSize, &buf);
        IDecipher((uint32_t*)&buf);

        // Copy the relevant portion to the output buffer
        memcpy(buffer, &buf[startChunkPos], startAmt);

        SetPosition(startPos+totalNumRead);
    }

    if (numMidChunks != 0)
    {
        uint32_t* bufferPos = (uint32_t*)(((char*)buffer)+startAmt);
        for (int i = 0; i < numMidChunks; i++)
        {
            // Decrypt chunk
            IDecipher(bufferPos);
            bufferPos += (kEncryptChunkSize / sizeof(uint32_t));
        }
    }

    if (endAmt != 0)
    {
        // Read in the final chunk and decrypt it
        char buf[kEncryptChunkSize];
        SetPosition(startPos + startAmt + numMidChunks*kEncryptChunkSize);
        uint32_t numRead = IRead(kEncryptChunkSize, &buf);
        IDecipher((uint32_t*)&buf);

        memcpy(((char*)buffer)+totalNumRead-endAmt, &buf, endAmt);
        
        SetPosition(startPos+totalNumRead);
    }

    // If we read into the padding at the end, update the total read to not include that
    if (totalNumRead > 0 && startPos + totalNumRead > fActualFileSize)
    {
        totalNumRead -= (startPos + totalNumRead) - fActualFileSize;
        SetPosition(fActualFileSize);
    }

    return totalNumRead;
}

uint32_t plEncryptedStream::Write(uint32_t bytes, const void* buffer)
{
    if (fOpenMode != kOpenWrite)
    {
        hsAssert(0, "Trying to write to a read stream");
        return 0;
    }

    return fRAMStream->Write(bytes, buffer);
}

bool plEncryptedStream::IWriteEncypted(hsStream* sourceStream, const wchar_t* outputFile)
{
    hsUNIXStream outputStream;

    if (!outputStream.Open(outputFile, L"wb"))
        return false;

    outputStream.Write(kMagicStringLen, kMagicString);

    // Save some space to write the file size at the end
    outputStream.WriteLE32(0);

    // Write out all the full size encrypted blocks we can
    char buf[kEncryptChunkSize];
    uint32_t amtRead;
    while ((amtRead = sourceStream->Read(kEncryptChunkSize, &buf)) == kEncryptChunkSize)
    {
        IEncipher((uint32_t*)&buf);
        outputStream.Write(kEncryptChunkSize, &buf);
    }

    // Pad with random data and write out the final partial block, if there is one
    if (amtRead > 0)
    {
        static bool seededRand = false;
        if (!seededRand)
        {
            seededRand = true;
            srand((unsigned int)time(nil));
        }

        for (int i = amtRead; i < kEncryptChunkSize; i++)
            buf[i] = rand();

        IEncipher((uint32_t*)&buf);

        outputStream.Write(kEncryptChunkSize, &buf);
    }

    // Write the original file size at the start
    uint32_t actualSize = sourceStream->GetPosition();
    outputStream.Rewind();
    outputStream.Skip(kMagicStringLen);
    outputStream.WriteLE32(actualSize);

    outputStream.Close();

    return true;
}

bool plEncryptedStream::FileEncrypt(const char* fileName)
{
    wchar_t* wFilename = hsStringToWString(fileName);
    bool ret = FileEncrypt(wFilename);
    delete [] wFilename;
    return ret;
}

bool plEncryptedStream::FileEncrypt(const wchar_t* fileName)
{
    hsUNIXStream sIn;
    if (!sIn.Open(fileName))
        return false;

    // Don't double encrypt any files
    if (ICheckMagicString(sIn.GetFILE()))
    {
        sIn.Close();
        return true;
    }
    sIn.Rewind();

    plEncryptedStream sOut;
    bool wroteEncrypted = sOut.IWriteEncypted(&sIn, L"crypt.dat");

    sIn.Close();
    sOut.Close();

    if (wroteEncrypted)
    {
        plFileUtils::RemoveFile(fileName);
        plFileUtils::FileMove(L"crypt.dat", fileName);
    }

    return true;
}

bool plEncryptedStream::FileDecrypt(const char* fileName)
{
    wchar_t* wFilename = hsStringToWString(fileName);
    bool ret = FileDecrypt(wFilename);
    delete [] wFilename;
    return ret;
}

bool plEncryptedStream::FileDecrypt(const wchar_t* fileName)
{
    plEncryptedStream sIn;
    if (!sIn.Open(fileName))
        return false;

    hsUNIXStream sOut;
    if (!sOut.Open(L"crypt.dat", L"wb"))
    {
        sIn.Close();
        return false;
    }

    char buf[1024];

    while (!sIn.AtEnd())
    {
        uint32_t numRead = sIn.Read(sizeof(buf), buf);
        sOut.Write(numRead, buf);
    }

    sIn.Close();
    sOut.Close();

    plFileUtils::RemoveFile(fileName);
    plFileUtils::FileMove(L"crypt.dat", fileName);

    return true;
}

bool plEncryptedStream::ICheckMagicString(FILE* fp)
{
    char magicString[kMagicStringLen+1];
    fread(&magicString, kMagicStringLen, 1, fp);
    magicString[kMagicStringLen] = '\0';
    return (hsStrEQ(magicString, kMagicString) || hsStrEQ(magicString, kOldMagicString));
}

bool plEncryptedStream::IsEncryptedFile(const char* fileName)
{
    wchar_t* wFilename = hsStringToWString(fileName);
    bool ret = IsEncryptedFile(wFilename);
    delete [] wFilename;
    return ret;
}

bool plEncryptedStream::IsEncryptedFile(const wchar_t* fileName)
{
    FILE* fp = hsWFopen(fileName, L"rb");
    if (!fp)
        return false;

    bool isEncrypted = ICheckMagicString(fp);

    fclose(fp);

    return isEncrypted;
}

hsStream* plEncryptedStream::OpenEncryptedFile(const char* fileName, bool requireEncrypted, uint32_t* cryptKey)
{
    wchar_t* wFilename = hsStringToWString(fileName);
    hsStream* ret = OpenEncryptedFile(wFilename, requireEncrypted, cryptKey);
    delete [] wFilename;
    return ret;
}

hsStream* plEncryptedStream::OpenEncryptedFile(const wchar_t* fileName, bool requireEncrypted, uint32_t* cryptKey)
{
#ifndef PLASMA_EXTERNAL_RELEASE
    requireEncrypted = false;
#endif

    bool isEncrypted = IsEncryptedFile(fileName);

    hsStream* s = nil;
    if (isEncrypted)
        s = new plEncryptedStream(cryptKey);
    // If this isn't an external release, let them use unencrypted data
    else
        if (!requireEncrypted)
            s = new hsUNIXStream;

    if (s)
        s->Open(fileName, L"rb");
    return s;
}

hsStream* plEncryptedStream::OpenEncryptedFileWrite(const char* fileName, uint32_t* cryptKey)
{
    wchar_t* wFilename = hsStringToWString(fileName);
    hsStream* ret = OpenEncryptedFileWrite(wFilename, cryptKey);
    delete [] wFilename;
    return ret;
}

hsStream* plEncryptedStream::OpenEncryptedFileWrite(const wchar_t* fileName, uint32_t* cryptKey)
{
    hsStream* s = nil;
#ifdef PLASMA_EXTERNAL_RELEASE
    s = new plEncryptedStream(cryptKey);
#else
    s = new hsUNIXStream;
#endif

    s->Open(fileName, L"wb");
    return s;
}
