import os

# lambdas = ['4'] + [str(l) for l in range(10,101,10)]
# lambdas = [str(2**i) for i in range(3,4)]
# lambdas = [str(l) for l in range(5,2049)]
lambdas = []
i = 5
inc = 1
while i <= 2048:
    lambdas.append(str(i))
    if 2**(inc+3) <= i:
        inc <<= 1
    i += inc
seeds = [str(s) for s in range(10)]

def main():
    user = 'user/nand/'
    cgt_short = 'cgt_auto.cfg'
    cgt = '{}{}'.format(user, cgt_short)
    size = 2
    time = 5*10**6 # us
    encfile = '{}encrypted.tmp'.format(user)
    decfile_short = 'decrypted.tmp'
    decfile = '{}{}'.format(user, decfile_short)
    timeCSV = 'time.csv'
    correctCSV = 'correct.csv'
    bdds = ['pil'] # ['N']#, 'A', 'AZ', 'F', 'FM']
    gates = ['NOT', 'AND', 'OR', 'XOR', 'NAND', 'NOR', 'XNOR', 'MUX']
    nbdds = len(bdds)
    nGates = len(gates)

    if not os.path.isfile(timeCSV):
        items = ['']
        for bdd in bdds:
            items += nGates * [bdd]
        writeRowCSV(filename=timeCSV, items=items)
        writeRowCSV(filename=timeCSV, items=['lambda'] + nbdds*gates)

    if not os.path.isfile(correctCSV):
        items = ['lambda'] + bdds
        writeRowCSV(filename=correctCSV, items=items)

    for l in lambdas:
        for seed in seeds:
            writeConfig(cgt_short, l, seed)
            compile(user, cgt)
            run(encfile, time)
            decrypt(encfile, decfile, cgt, size)
            writeRowCSV(correctCSV, [l]+loadCorrectnessResults(decfile_short, nbdds))
            writeRowCSV(timeCSV, [l]+loadTimingResults(decfile_short, nGates))

def compile(user, cgt):
    # command = 'cd ../.. && make c && rm -rf *key* x.* && make USER={} CGT={} CUDD=1'.format(user, cgt)
    # command = 'cd ../.. && make c && rm -rf *.key && make USER={} CGT={} MPIR=1'.format(user, cgt)
    command = 'cd ../.. && rm -rf *.key secint.* && make USER={} CGT={} MPIR=1'.format(user, cgt)
    os.system(command)

def decrypt(encfile, decfile, cgt, size):
    command = 'cd ../.. && ./cgt.exe dec -c {} -s {} < {} > {}'.format(cgt, size, encfile, decfile)
    os.system(command)

def loadCorrectnessResults(filename, nbdds):
    lout = []
    with open(filename, 'r') as fin:
        lines = fin.read().splitlines()
        lines = lines[-nbdds : ]
        print(lines)
        lout = [l[-1] for l in lines]
    return lout

def loadTimingResults(filename, nGates):
    lout = []
    with open(filename, 'r') as fin:
        lines = fin.read().splitlines()
        indexes = [lines.index(l) for l in lines if 'bdd' in l]
        for idx in indexes:
            for g in range(nGates):
                lout.append( lines[idx+1+g].split(' ')[-1] )
    return lout

def run(filename, time):
    command = 'cd ../.. && ./bob.exe > {} {}'.format(filename, time)
    os.system(command)

def writeConfig(filename, l, seed):
    with open(filename, 'w') as fout:
        cgt = """#generated by testGates.py
password = hello_world"""+seed+"""

@kernel = bbs
@circuitDB = tfhe
# @formula = FLF
@sizes = 2
@lambda = """+l+"""
@compile = [ unx: g++ $.cpp -o $.exe ][ win: cl -EHsc -Ox -nologo $.cpp ]

BN : circuit
{
	postfix = En
	encryption = pil
}
        """
        fout.write(cgt)

def writeRowCSV(filename, items):
    with open(filename, 'a') as fout:
        strout = ''
        for item in items:
            strout += item+','
        fout.write('{}\n'.format(strout[:-1]))

if __name__ == '__main__':
    main()
