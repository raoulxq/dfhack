/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix), Kenneth Ferland (Impaler[WrG]), dorf

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/
#include "Internal.h"
#include "PlatformInternal.h"

#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <cstring>
using namespace std;

#include "LinuxProcess.h"
#include "ProcessFactory.h"
#include "dfhack/VersionInfo.h"
#include "dfhack/DFError.h"
#include <errno.h>
#include <sys/ptrace.h>
#include <md5wrapper.h>
using namespace DFHack;

namespace {
    class NormalProcess : public LinuxProcessBase
    {
        private:
            uint8_t vector_start;
        public:
            NormalProcess(uint32_t pid, VersionInfoFactory * known_versions);
            ~NormalProcess()
            {
                if(attached)
                {
                    detach();
                }
            }
            bool attach();
            bool detach();

            bool suspend();
            bool asyncSuspend();
            bool resume();
            bool forceresume();

            void readSTLVector(const uint32_t address, t_vecTriplet & triplet);
            const std::string readSTLString (uint32_t offset);
            size_t readSTLString (uint32_t offset, char * buffer, size_t bufcapacity);
            size_t writeSTLString(const uint32_t address, const std::string writeString);
            size_t copySTLString(const uint32_t address, const uint32_t target);
            // get class name of an object with rtti/type info
            std::string readClassName(uint32_t vptr);
    };
}

Process* DFHack::createNormalProcess(uint32_t pid, VersionInfoFactory * known_versions)
{
    return new NormalProcess(pid, known_versions);
}

NormalProcess::NormalProcess(uint32_t pid, VersionInfoFactory * known_versions) : LinuxProcessBase(pid)
{
    char dir_name [256];
    char exe_link_name [256];
    char mem_name [256];
    char cwd_name [256];
    char cmdline_name [256];
    char target_name[1024];
    int target_result;

    identified = false;
    my_descriptor = 0;

    sprintf(dir_name,"/proc/%d/", pid);
    sprintf(exe_link_name,"/proc/%d/exe", pid);
    sprintf(mem_name,"/proc/%d/mem", pid);
    memFile = mem_name;
    sprintf(cwd_name,"/proc/%d/cwd", pid);
    sprintf(cmdline_name,"/proc/%d/cmdline", pid);

    // resolve /proc/PID/exe link
    target_result = readlink(exe_link_name, target_name, sizeof(target_name)-1);
    if (target_result == -1)
    {
        return;
    }
    // make sure we have a null terminated string...
    target_name[target_result] = 0;

    // is this the regular linux DF?
    if (strstr(target_name, "dwarfort.exe") != 0 || strstr(target_name,"Dwarf_Fortress") != 0)
    {
        md5wrapper md5;
        // get hash of the running DF process
        string hash = md5.getHashFromFile(target_name);
        // create linux process, add it to the vector
        VersionInfo * vinfo = known_versions->getVersionInfoByMD5(hash);
        if(vinfo)
        {
            my_descriptor = new VersionInfo(*vinfo);
            my_descriptor->setParentProcess(this);
            vector_start = my_descriptor->getGroup("vector")->getOffset("start");
            identified = true;
        }
    }
}

struct _Rep_base
{
    uint32_t _M_length; // length of text stored, not including zero termination
    uint32_t _M_capacity; // capacity, not including zero termination
    uint32_t _M_refcount; // reference count (two STL strings can share a common buffer, copy on write rules apply)
};

size_t NormalProcess::readSTLString (uint32_t offset, char * buffer, size_t bufcapacity)
{
    _Rep_base header;
    offset = Process::readDWord(offset);
    read(offset - sizeof(_Rep_base),sizeof(_Rep_base),(uint8_t *)&header);
    size_t read_real = min((size_t)header._M_length, bufcapacity-1);// keep space for null termination
    read(offset,read_real,(uint8_t * )buffer);
    buffer[read_real] = 0;
    return read_real;
}

size_t NormalProcess::writeSTLString(const uint32_t address, const std::string writeString)
{
    _Rep_base header;
    // get buffer location
    uint32_t start = Process::readDWord(address);
    // read the header
    read(start - sizeof(_Rep_base),sizeof(_Rep_base),(uint8_t *)&header);

    // the buffer has actual size = 1. no space for storing anything more than a zero byte
    if(header._M_capacity == 0)
        return 0;
    if(header._M_refcount != 0 && header._M_refcount != 0xFFFFFFFF ) // one ref or one non-shareable ref
        return 0;

    // get writeable length (lesser of our string length and capacity of the target)
    uint32_t lstr = writeString.length();
    uint32_t allowed_copy = min(lstr, header._M_capacity);
    // write header with new length.
    header._M_length = allowed_copy;
    write(start - sizeof(_Rep_base),sizeof(_Rep_base),(uint8_t *)&header);
    // write string, add a zero terminator, return bytes written
    write(start, allowed_copy, (uint8_t *) writeString.c_str());
    writeByte(start + allowed_copy, 0);
    return allowed_copy;
}

void NormalProcess::readSTLVector(const uint32_t address, t_vecTriplet & triplet)
{
    read(address + vector_start, sizeof(triplet), (uint8_t *) &triplet);
}

const string NormalProcess::readSTLString (uint32_t offset)
{
    _Rep_base header;

    offset = Process::readDWord(offset);
    read(offset - sizeof(_Rep_base),sizeof(_Rep_base),(uint8_t *)&header);

    // FIXME: use char* everywhere, avoid string
    char * temp = new char[header._M_length+1];
    read(offset,header._M_length+1,(uint8_t * )temp);
    string ret(temp);
    delete temp;
    return ret;
}

size_t NormalProcess::copySTLString (uint32_t offset, uint32_t target)
{
    _Rep_base header;

    offset = Process::readDWord(offset);
    uint32_t old_target = Process::readDWord(target);

    if (offset == old_target)
        return 0;

    read(offset - sizeof(_Rep_base),sizeof(_Rep_base),(uint8_t *)&header);

    // destroying the leaked state
    if (header._M_refcount == -1)
        header._M_refcount = 1;
    else
        header._M_refcount++;

    write(offset - sizeof(_Rep_base),sizeof(_Rep_base),(uint8_t *)&header);

    writeDWord(target, offset);
    return header._M_length;
}

string NormalProcess::readClassName (uint32_t vptr)
{
    int typeinfo = Process::readDWord(vptr - 0x4);
    int typestring = Process::readDWord(typeinfo + 0x4);
    string raw = readCString(typestring);
    size_t  start = raw.find_first_of("abcdefghijklmnopqrstuvwxyz");// trim numbers
    size_t end = raw.length();
    return raw.substr(start,end-start);
}

bool NormalProcess::asyncSuspend()
{
    return suspend();
}

bool NormalProcess::suspend()
{
    int status;
    if(!attached)
        return false;
    if(suspended)
        return true;
    if (kill(my_pid, SIGSTOP) == -1)
    {
        // no, we got an error
        perror("kill SIGSTOP error");
        return false;
    }
    while(true)
    {
        // we wait on the pid
        pid_t w = waitpid(my_pid, &status, 0);
        if (w == -1)
        {
            // child died
            perror("DF exited during suspend call");
            return false;
        }
        // stopped -> let's continue
        if (WIFSTOPPED(status))
        {
            break;
        }
    }
    suspended = true;
    return true;
}

bool NormalProcess::forceresume()
{
    return resume();
}

bool NormalProcess::resume()
{
    if(!attached)
        return false;
    if(!suspended)
        return true;
    if (ptrace(PTRACE_CONT, my_pid, NULL, NULL) == -1)
    {
        // no, we got an error
        perror("ptrace resume error");
        return false;
    }
    suspended = false;
    return true;
}

bool NormalProcess::attach()
{
    int status;
    if(attached)
    {
        if(!suspended)
            return suspend();
        return true;
    }
    // can we attach?
    if (ptrace(PTRACE_ATTACH , my_pid, NULL, NULL) == -1)
    {
        // no, we got an error
        perror("ptrace attach error");
        cerr << "attach failed on pid " << my_pid << endl;
        return false;
    }
    while(true)
    {
        // we wait on the pid
        pid_t w = waitpid(my_pid, &status, 0);
        if (w == -1)
        {
            // child died
            perror("wait inside attach()");
            return false;
        }
        // stopped -> let's continue
        if (WIFSTOPPED(status))
        {
            break;
        }
    }
    suspended = true;

    int proc_pid_mem = open(memFile.c_str(),O_RDONLY);
    if(proc_pid_mem == -1)
    {
        ptrace(PTRACE_DETACH, my_pid, NULL, NULL);
        cerr << memFile << endl;
        cerr << "couldn't open /proc/" << my_pid << "/mem" << endl;
        perror("open(memFile.c_str(),O_RDONLY)");
        return false;
    }
    else
    {
        attached = true;

        memFileHandle = proc_pid_mem;
        return true; // we are attached
    }
}

bool NormalProcess::detach()
{
    if(!attached) return true;
    if(!suspended) suspend();
    int result = 0;
    // close /proc/PID/mem
    result = close(memFileHandle);
    if(result == -1)
    {
        cerr << "couldn't close /proc/"<< my_pid <<"/mem" << endl;
        perror("mem file close");
        return false;
    }
    else
    {
        // detach
        result = ptrace(PTRACE_DETACH, my_pid, NULL, NULL);
        if(result == -1)
        {
            cerr << "couldn't detach from process pid" << my_pid << endl;
            perror("ptrace detach");
            return false;
        }
        else
        {
            attached = false;
            return true;
        }
    }
}
