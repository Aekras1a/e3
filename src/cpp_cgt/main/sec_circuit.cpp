#include <set>

#include "cfgparser.h" // ConfigNames
#include "secnames.h"
#include "sec_bridge.h"
#include "sec_circuit.h"

using namespace e3;

Circuit::Circuit(std::istream & is, string nm, const std::map<string, string> & globs) : SecType(nm), modifier(0)
{
    std::map<string, string *> kv;
    kv[secNames::postfix] = &postfixP;
    kv[secNames::postneg] = &postfixN;
    kv[secNames::encryption] = &encType;
    kv[secNames::circDb] = &circDb;
    kv[secNames::formula] = &formula;
    kv[secNames::compile] = &compile;
    kv[secNames::kernel] = &kernel;

    string ssizes;
    kv[secNames::sizes] = &ssizes;

    string slambda;
    kv[secNames::lambda] = &slambda;

    loadPairs(is, kv);
    globPairs(kv, globs);

    fixEncType();

    if ( encType.empty() ) throw "encryption type must be defined for " + nm;
    else if ( encType == secNames::encPlain ) {}
    else if ( basType == secNames::encBdd ) {}
    else if ( encType == secNames::encFhew ) {}
    else if ( encType == secNames::encHeli ) {}
    else if ( basType == secNames::encPilc ) {}
    else if ( encType == secNames::encSeal ) {}
    else if ( encType == secNames::encTfhe ) {}
    else if ( encType == secNames::encExt ) {}
    else if ( encType[0] == '@' ) {}
    else throw "encryption type ["
        + encType + "] is not known; valid="
        "(plain,tfhe,seal,fhew,heli,bddn,bddnx,bdda,"
        "bddaz,bddf,bddfm,pil,extern)";

    if ( circDb.empty() ) circDb = encType;

    if ( !formula.empty() && encType != secNames::encBddn
            && encType != secNames::encBdda && encType != secNames::encBddf )
        std::cout << "Warning: formula ignored for type: " << encType << '\n';

    // parse ssizes;
    sizes.push_back(1);
    ol::istr sis(ssizes);
    for ( string s; std::getline(sis, s, ','); )
    {
        auto dash = s.find('-');
        if ( dash == string::npos )
        {
            sizes.push_back(std::stoi(s));
            continue;
        }

        int a = std::stoi(s.substr(0, dash));
        int b = std::stoi(s.substr(dash + 1));

        if (a <= b) for ( int i = a; i <= b; i++ ) sizes.push_back(i);
    }

    // set max plaintext size
    for ( auto x : sizes ) if ( x > plaintext_size ) plaintext_size = x;

    if ( encType[0] == '@' )
    {
        if ( !slambda.empty() ) cout << "Warning: lambda is ignored for " << nm << '\n';
    }
    else
    {
        if ( slambda.empty() ) lambda = 1;
        else lambda = std::stoi(slambda);

        if ( basType == secNames::encBdd )
        {
            const int deflam = 3;
            if ( slambda.empty() )
            {
                lambda = deflam;
                std::cout << "Warning: lambda is not defined for "
                          << nm << ", using " << lambda << '\n';
            }
            if ( lambda < deflam )
            {
                lambda = deflam;
                std::cout << "Warning: lambda cannot be <3 for "
                          << nm << ", using " << lambda << '\n';
            }
        }
    }
}

void Circuit::genKeys(bool forceGen, bool forceLoad,
                      std::string seed, const ConfigParser * par)
{
    // make seed different for different secure types
    seed = name.typ + seed;

    // FIXME this is ugly, it should be some kind of array
    // or something generic, so we dont have to keep adding code as we add schemes
    if (0) {}

    else if ( encType == secNames::encBddn )
        sk = shared_ptr<PrivKey>
             (csk = new CircuitPrivKey_bddn
        (name, forceGen, forceLoad, seed,
         lambda, formula, compile, kernel, modifier ));

    else if ( encType == secNames::encBdda )
        sk = shared_ptr<PrivKey>
             (csk = new CircuitPrivKey_bdda
        (name, forceGen, forceLoad, seed,
         lambda, formula, compile, kernel, modifier ));

    else if ( encType == secNames::encBddf )
        sk = shared_ptr<PrivKey>
             (csk = new CircuitPrivKey_bddf
        (name, forceGen, forceLoad, seed,
         lambda, formula, compile, kernel, modifier ));

    else if ( encType == secNames::encHeli )
        sk = shared_ptr<PrivKey>
             (csk = new CircuitPrivKey_heli(name, forceGen,
                                            forceLoad, seed, lambda));

    else if ( encType == secNames::encFhew )
        sk = shared_ptr<PrivKey>
             (csk = new CircuitPrivKey_fhew(name, forceGen,
                                            forceLoad, seed, lambda));

    else if ( encType == secNames::encSeal )
        sk = shared_ptr<PrivKey>
             (csk = new CircuitPrivKey_seal(name, forceGen,
                                            forceLoad, seed, lambda));

    else if ( encType == secNames::encTfhe )
        sk = shared_ptr<PrivKey>
             (csk = new CircuitPrivKey_tfhe(name, forceGen,
                                            forceLoad, seed, lambda));

    else if ( encType == secNames::encPlain )
        sk = shared_ptr<PrivKey>
             (csk = new CircuitPrivKey_plain(name, forceGen,
                                             forceLoad, seed));

    else if ( encType == secNames::encPilc )
        sk = shared_ptr<PrivKey>
             (csk = new CircuitPrivKey_pilc(name, forceGen,
                                            forceLoad, seed, lambda));

    else if ( encType == secNames::encExt )
        sk = shared_ptr<PrivKey>
             (csk = new CircuitPrivKey_ext());

    else if ( encType[0] == '@' )
    {
        makeBridge(par, 0); if ( !bridge ) never("bridge");

        if (0) {}

        else if ( encType == secNames::encPilc )
        {
            PilBasePrivKey * p = dynamic_cast<PilBasePrivKey *>(bridge->get_sk_raw());
            if ( !p ) never("bad bridge");
            sk = shared_ptr<PrivKey>(csk = new CircuitPrivKey_pilc(*p, name.typ));
        }

        else
            throw "Circuit: Bridge is not supperted for type ["
            + encType + "] in " + name.typ;
    }

    else
        throw "Circuit: Bad encryption type ["
        + encType + "] in " + name.typ;
}

void Circuit::writeH(string root, std::ostream & os, string user_dir) const
{

    // write base Bit class
    {
        string dbf = cfgNames::dotH(cfgNames::dbfileCircuitBase);
        string f = ol::file2str(root + cfgNames::templDir + dbf);
        ol::replaceAll(f, secNames::R_TypName, name.typ);
        ol::replaceAll(f, secNames::R_FilName, name.fil);
        ol::replaceAll(f, secNames::R_ClsName, encType);
        os << f;
    }

    // write Bit class defintion
    {
        string dbf = cfgNames::dotH(
                         cfgNames::dbfileCircuit + '.' + encType);

        string f = ol::file2str(root + cfgNames::templDir + dbf);

        if ( !f.empty() && f[0] == '@' )
        {
            f = f.substr(1);
            ol::cutline(f);
            f = ol::file2str(root + cfgNames::templDir + f);
        }

        ol::replaceAll(f, secNames::R_TypName, name.typ);
        ol::replaceAll(f, secNames::R_FilName, name.fil);
        ol::replaceAll(f, secNames::R_ClsName, encType);
        ol::replaceAll(f, secNames::R_lambda, ol::tos(lambda) );
        os << f;
    }


    {
        string dbf = cfgNames::dotH(cfgNames::dbfileCircuit);
        string f = ol::file2str(root + cfgNames::templDir + dbf);
        ol::replaceAll(f, secNames::R_TypName, name.typ);
        ol::replaceAll(f, secNames::R_FilName, name.fil);
        ol::replaceAll(f, secNames::R_ClsName, encType);

        auto allconsts = find_constants(user_dir);
        ol::replaceAll(f, secNames::R_postfixDefines, makeDefines(allconsts) );

        os << f;
    }
}

void Circuit::writeInc(string root, std::ostream & os) const
{
    {
        string dbf = cfgNames::dotInc(cfgNames::dbfileCircuit);
        string f = ol::file2str(root + cfgNames::templDir + dbf);
        ol::replaceAll(f, secNames::R_TypName, name.typ);
        ol::replaceAll(f, secNames::R_FilName, name.fil);
        ol::replaceAll(f, secNames::R_ClsName, encType);
        ol::replaceAll(f, secNames::R_lambda, ol::tos(lambda) );
        os << f;
    }
}

void Circuit::writeCpp(string root, std::ostream & os) const
{
    // write circuits
    std::set<int> done;
    for ( auto i : sizes )
    {
        if ( done.find(i) != done.end() ) continue;
        done.insert(i);

        string path = cfgNames::circDirt + circDb + "/" + ol::tos(i)
                      + "/" + cfgNames::circuitFilename;

        string cf = ol::file2str(root + path);
        ol::replaceAll(cf, secNames::R_TypName, name.typ);
        ol::replaceAll(cf, secNames::R_FilName, name.fil);
        ol::replaceAll(cf, secNames::R_ClsName, encType);
        ol::replaceAll(cf, secNames::R_lambda, ol::tos(lambda) );
        os << cf;
    }

    // write class globals and non-template function
    {
        string dbf = cfgNames::dotCpp(cfgNames::dbfileCircuit);

        string f = ol::file2str(root + cfgNames::templDir + dbf);
        ol::replaceAll(f, secNames::R_TypName, name.typ);
        ol::replaceAll(f, secNames::R_FilName, name.fil);
        ol::replaceAll(f, secNames::R_ClsName, encType);
        ol::replaceAll(f, secNames::R_lambda, ol::tos(lambda) );
        ol::replaceAll(f, secNames::R_bitZero, csk->encbitstr(false) );
        ol::replaceAll(f, secNames::R_bitUnit, csk->encbitstr(true) );
        os << f;
    }

    // write gate implementations
    {
        string ver;
        int iver = -1;
        if ( encType == secNames::encFhew ) iver = fhew_impl();
        if ( encType == secNames::encHeli ) iver = heli_impl();
        if ( encType == secNames::encSeal ) iver = seal_impl();
        if ( encType == secNames::encTfhe ) iver = tfhe_impl();
        if ( iver != -1 ) ver = ol::tos(iver);

        if (basType != encType)
        {
            string dbf = cfgNames::dotCpp(cfgNames::dbfileCircuit
                                          + '.' + basType + ver);

            string f = ol::file2str(root + cfgNames::templDir + dbf);
            ol::replaceAll(f, secNames::R_TypName, name.typ);
            ol::replaceAll(f, secNames::R_FilName, name.fil);
            ol::replaceAll(f, secNames::R_ClsName, encType);
            ol::replaceAll(f, secNames::R_lambda, ol::tos(lambda));
            ol::replaceAll(f, secNames::R_x2lambda, ol::tos(2 * lambda));
            ol::replaceAll(f, secNames::R_tid, csk->getTid());
            ol::replaceAll(f, secNames::R_Modifier, ol::tos(modifier));
            os << f;
        }

        // FIXME e factor out repetition of replacing
        {
            string dbf = cfgNames::dotCpp(cfgNames::dbfileCircuit
                                          + '.' + encType + ver);

            string f = ol::file2str(root + cfgNames::templDir + dbf);
            ol::replaceAll(f, secNames::R_TypName, name.typ);
            ol::replaceAll(f, secNames::R_FilName, name.fil);
            ol::replaceAll(f, secNames::R_ClsName, encType);
            ol::replaceAll(f, secNames::R_lambda, ol::tos(lambda) );
            ol::replaceAll(f, secNames::R_x2lambda, ol::tos(2 * lambda) );
            ol::replaceAll(f, secNames::R_tid, csk->getTid());
            ol::replaceAll(f, secNames::R_Modifier, ol::tos(modifier) );
            os << f;
        }
    }
}

void Circuit::fixEncType()
{
    if (encType == secNames::encBddfm) { encType = secNames::encBddf; modifier = 1; }
    if (encType == secNames::encBddaz) { encType = secNames::encBdda; modifier = 1; }
    if (encType == secNames::encBddnx) { encType = secNames::encBddn; modifier = 1; }
    if (encType == secNames::encPilBase ) { encType = secNames::encPilc; }

    if ( encType == secNames::encBddn
            || encType == secNames::encBdda
            || encType == secNames::encBddf )
        basType = secNames::encBdd;
    else
        basType = encType;
}
