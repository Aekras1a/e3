#include <iostream>
#include <string>

using std::string;
using std::cout;

#include "euler.h"

Bigun euler::prime(int lam, Rnd * rnd, Bigun skip)
{
    const bool PR1 = false;
    const bool PR2 = true;

    if ( lam < 5 ) throw "Cannot generate prime with bitsize <5";
    auto lm = lam - 1;

    Bigun x(0), u1111(1), u1011(1), u1000(1);

    // get patterns
    for ( int i = 1; i < lam; i++ ) u1111 = 2 * u1111 + 1;
    u1011 <<= lm;
    u1000 <<= lm;
    u1011 |= 3;

    if (PR1) cout << "Lam: " << lam << '\n';
    if (PR1) cout << "Pat: " << u1111 << ' ' << u1011 << ' ' << u1000 << '\n';

    if (PR2) cout << "Gen prime ";
    for ( int i = 0; i < 1000; i++ ) // try 1000 times
    {
        x = 1;
        for ( int j = 1; j < lam; j++ ) x = 2 * x + ( ((*rnd)() >> 8) & 1 );
        x |= u1011;
        if (PR1) cout << x << '\n';
        while (1)
        {
            if ( x == u1111 ) break;
            if ( x != skip && isprime(x) )
            {
                if (PR2) cout << " ok\n";
                return x;
            }
            x += 4;
            if ( !(x >> lm) ) break;
            if (PR1) cout << "try " << x << '\n';
            if (PR2) cout << '.' << std::flush;
        }
        if (PR1) cout << "trying new random ";
    }

    throw "Not able to generate prime";
}

static bool isPrime(Bigun x, const std::vector<Bigun> & v)
{
    for ( const auto & i : v )
        if ( i != i.powmod(x, x) ) return false;

    return true;
}

static std::vector<Bigun> genPrimes()
{
    std::vector<Bigun> v = {2};

    while ( v.size() < TESTNPRIMES )
    {
        auto x = v.back();

        if ( v.size() == 1 ) x = 3;
        else x += 2;

        while ( !isPrime(x, v) ) x += 2;
        v.push_back(x);
    }

    return v;
}

bool euler::isprime(Bigun x)
{
    static std::vector<Bigun> vprimes = genPrimes();
    return isPrime(x, vprimes);
}

Bigun euler::random(Bigun mod, Rnd * rnd)
{
    Bigun x = ( (*rnd)() >> 8 );
    while (x < mod) { x <<= 8; x += (*rnd)(); }
    return x % mod;
}

