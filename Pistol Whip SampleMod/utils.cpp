// thx https://github.com/jbro129/Unity-Substrate-Hook-Android


#include <sys/types.h>
#include <sys/stat.h>
#include "utils-functions.hpp"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

#include "logger.h"

using namespace std;


long long location; // save lib.so base address so we do not have to recalculate every time causing lag.



void setcsstr(Il2CppString* in, u16string_view str) {
    in->length = str.length();
    for(int i = 0; i < in->length; i++) {
        // Can assume that each char is only a single char (a single word --> double word)
        in->chars[i] = str[i];
    }
}

string to_utf8(u16string_view view) {
    char *dat = new char[view.length() + 1];

    transform(view.data(), view.data() + view.size(), dat, [](auto utf16_char) {
        return static_cast<char>(utf16_char);
    });
    dat[view.length()] = '\0';
    string ret(dat);
    delete[] dat;
    return ret;
}

u16string to_utf16(string_view view) {
    char16_t *dat = new char16_t[view.length() + 1];
    transform(view.data(), view.data() + view.size(), dat, [](auto standardChar) {
        return static_cast<char16_t>(standardChar);
    });
    dat[view.length()] = '\0';
    u16string ret(dat);
    delete[] dat;
    return ret;
}

u16string_view csstrtostr(Il2CppString* in)
{
    return u16string_view((char16_t*)in->chars, static_cast<uint32_t>(in->length));
}

void dump(int before, int after, void* ptr) {
    LOG("Dumping Immediate Pointer: %p: %08x", ptr, *reinterpret_cast<int*>(ptr));
    auto begin = static_cast<int*>(ptr) - before;
    auto end = static_cast<int*>(ptr) + after;
    for (auto cur = begin; cur != end; ++cur) {
        LOG("%p: %08x", cur, *cur);
    }
}

//TODO DO WE NEED THIS?
//bool fileexists(const char* filename) {
//    return access(filename, W_OK | R_OK) != -1;
//}

bool direxists(const char* dirname) {
    struct stat info;

    if (stat(dirname, &info) != 0) {
        return false;
    } if (info.st_mode & S_IFDIR) {
        return true;
    }
    return false;
}

char* readfile(const char* filename) {
    FILE* fp;
    fopen_s(&fp, filename, "r");
    char* content = NULL;
    long size = 0;
    if (fp) {
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        rewind(fp);
        content = (char*)malloc(size * sizeof(char));
        fread(content, sizeof(char), size, fp);
        fclose(fp);
    }
    return content;
}

int writefile(const char* filename, const char* text) {
    FILE* fp;
    fopen_s(&fp, filename, "w");
    if (fp) {
        fwrite(text, sizeof(char), strlen(text), fp);
        fclose(fp);
        return 0;
    }
    return -1;
}
