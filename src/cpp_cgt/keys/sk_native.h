#pragma once

#include "sk_abstract.h"
#include "ekx_native.h"
#include "e3util.h"

namespace e3
{

class NativePrivKey : public PrivKey
{
        virtual void gen();

    private:
        e3util::ull key;
        NativeEvalKeyExt ek; // decryption and encryption are there

    public:
        NativePrivKey(KeyName name, bool forceGen,
                      bool forceLoad, std::string seed);

        virtual std::string decrypt(const std::string & s) const;
        virtual std::string encrypt(const std::string & s, int msz) const;
        ///NativeEvalKey & getEk() { return ek; }
        virtual bool load();
        virtual void save();
};

} // e3
