
import re
import os
import sys
import time
import random
import argparse
import subprocess
import math

# define the constants
ACCURACY = 20
FACTOR_DIGITS = 100
BUF_SIZE = 1024
BIGNUM_CAPACITY = 20
RADIX = 4294967296
HALFRADIX = 2147483648
UINT_MAX = sys.maxsize
RAND_MAX = EXPONENT_MAX = 2147483647


def MAX(a, b):
    return a if a > b else b


class bignum:
    def __init__(self, capacity=BIGNUM_CAPACITY, length=0, data=None):
        self.capacity = capacity
        self.length = length
        self.data = data


NUMS = [
    {1, 1, 0}, {1, 1, 1}, {1, 1, 2}, {1, 1, 3}, {1, 1, 4}, {1, 1, 5}, {
        1, 1, 6}, {1, 1, 7}, {1, 1, 8}, {1, 1, 9}, {1, 1, 10}
]


def bignum_init():
    return bignum()


def bignum_deinit(bn):
    bn.capacity = 0
    bn.length = 0
    bn.data = None


def bignum_iszero(bn):
    return bn.length == 0 or (bn.length == 1 and bn.data[0] == 0)


def bignum_isnotzero(bn):
    return bignum_iszero(bn) == False


def bignum_copy(bn):
    newbn = bignum_init()
    newbn.length = bn.length
    newbn.capacity = bn.capacity
    newbn.data = bn.data
    return newbn


def bignum_fromstring(bn, string):
    i = 0
    length = 0
    while length < len(string) and string[length] != '\0':
        length += 1
    for i in range(length):
        if i != 0:
            bignum_imultiply(bn, NUMS[10])
        # bignum_iadd(bn, NUMS[string[i] - '0'])
            bignum_iadd(bn, NUMS[ord(string[i]) - ord('0')])


def bignum_fromint(bn, num):
    bn.length = 1
    if bn.capacity < bn.length:
        bn.capacity = bn.length
        bn.data = [0] * bn.capacity
    bn.data[0] = num


def bignum_print(bn):
    cap = 100
    length = 0
    buffer = [0] * cap
    copy = bignum_init()
    remainder = bignum_init()
    if bn.length == 0 or bignum_iszero(bn):
        print("0")
    else:
        bignum_copy(bn, copy)
        while bignum_isnotzero(copy):
            bignum_idivide(copy, NUMS[10], remainder)
            buffer[length] = remainder.data[0]
            length += 1
            if length >= cap:
                cap *= 2
                buffer = [0] * cap
        for i in range(length - 1, -1, -1):
            print(buffer[i], end='')
    bignum_deinit(copy)
    bignum_deinit(remainder)


def bignum_equal(b1, b2):
    if bignum_iszero(b1) and bignum_iszero(b2):
        return 1
    elif bignum_iszero(b1):
        return 0
    elif bignum_iszero(b2):
        return 0
    elif b1.length != b2.length:
        return 0
    for i in range(b1.length - 1, -1, -1):
        if b1.data[i] != b2.data[i]:
            return 0
    return 1


def bignum_greater(b1, b2):
    if bignum_iszero(b1) and bignum_iszero(b2):
        return 0
    elif bignum_iszero(b1):
        return 0
    elif bignum_iszero(b2):
        return 1
    elif b1.length != b2.length:
        return b1.length > b2.length
    for i in range(b1.length - 1, -1, -1):
        if b1.data[i] != b2.data[i]:
            return b1.data[i] > b2.data[i]
    return 0


def bignum_less(b1, b2):
    if bignum_iszero(b1) and bignum_iszero(b2):
        return 0
    elif bignum_iszero(b1):
        return 1
    elif bignum_iszero(b2):
        return 0
    elif b1.length != b2.length:
        return b1.length < b2.length
    for i in range(b1.length - 1, -1, -1):
        if b1.data[i] != b2.data[i]:
            return b1.data[i] < b2.data[i]
    return 0


def bignum_geq(b1, b2):
    return bignum_less(b1, b2) == False


def bignum_leq(b1, b2):
    return bignum_greater(b1, b2) == False


def bignum_iadd(source, add):
    temp = bignum_init()
    bignum_add(temp, source, add)
    bignum_copy(temp, source)
    bignum_deinit(temp)


def bignum_add(result, b1, b2):
    sum = 0
    carry = 0
    n = max(b1.length, b2.length)
    if n + 1 > result.capacity:
        result.capacity = n + 1
        result.data = [0] * result.capacity
    for i in range(n):
        sum = carry
        if i < b1.length:
            sum += b1.data[i]
        if i < b2.length:
            sum += b2.data[i]
        result.data[i] = sum

        if i < b1.length:
            if sum < b1.data[i]:
                carry = 1
            else:
                carry = 0
        else:
            if sum < b2.data[i]:
                carry = 1
            else:
                carry = 0
    if carry == 1:
        result.length = n + 1
        result.data[n] = 1
    else:
        result.length = n


def bignum_isubtract(source, sub):
    temp = bignum_init()
    bignum_subtract(temp, source, sub)
    bignum_copy(temp, source)
    bignum_deinit(temp)


def bignum_subtract(result, b1, b2):
    length = 0
    carry = 0
    diff = 0
    temp = 0
    if b1.length > result.capacity:
        result.capacity = b1.length
        result.data = [0] * result.capacity
    for i in range(b1.length):
        temp = carry
        if i < b2.length:
            temp = temp + b2.data[i]
        diff = b1.data[i] - temp
        if temp > b1.data[i]:
            carry = 1
        else:
            if temp == 0 and b2.data[i] == 0xffffffff:
                carry = 1
            else:
                carry = 0
        result.data[i] = diff
        if result.data[i] != 0:
            length = i + 1
    result.length = length


def bignum_imultiply(source, mult):
    temp = bignum_init()
    bignum_multiply(temp, source, mult)
    bignum_copy(temp, source)
    bignum_deinit(temp)


def bignum_multiply(result, b1, b2):
    i = 0
    j = 0
    k = 0
    carry = 0
    temp = 0
    prod = 0
    if b1.length + b2.length > result.capacity:
        result.capacity = b1.length + b2.length
        result.data = [0] * result.capacity
    for i in range(b1.length + b2.length):
        result.data[i] = 0
    for i in range(b1.length):
        for j in range(b2.length):
            prod = b1.data[i] * b2.data[j] + result.data[i + j]
            carry = prod // RADIX
            k = 1
            while carry > 0:
                temp = result.data[i + j + k] + carry
                if temp < result.data[i + j + k]:
                    carry = 1
                else:
                    carry = 0
                result.data[i + j + k] = temp
                k += 1
            prod = (result.data[i + j] + b1.data[i] * b2.data[j]) % RADIX
            result.data[i + j] = prod
    if b1.length + b2.length > 0 and result.data[b1.length + b2.length - 1] == 0:
        result.length = b1.length + b2.length - 1
    else:
        result.length = b1.length + b2.length


def bignum_idivide(source, div):
    q = bignum_init()
    r = bignum_init()
    bignum_divide(q, r, source, div)
    bignum_copy(q, source)
    bignum_deinit(q)
    bignum_deinit(r)


def bignum_idivider(source, div, remainder):
    q = bignum_init()
    r = bignum_init()
    bignum_divide(q, r, source, div)
    bignum_copy(q, source)
    bignum_copy(r, remainder)
    bignum_deinit(q)
    bignum_deinit(r)


def bignum_remainder(source, div, remainder):
    q = bignum_init()
    bignum_divide(q, remainder, source, div)
    bignum_deinit(q)


def bignum_imodulate(source, modulus):
    q = bignum_init()
    r = bignum_init()
    bignum_divide(q, r, source, modulus)
    bignum_copy(r, source)
    bignum_deinit(q)
    bignum_deinit(r)


def bignum_divide(b1, b2, quotient, remainder):
    b2copy = bignum_init()
    b1copy = bignum_init()
    quottemp = bignum_init()
    temp = bignum_init()
    temp2 = bignum_init()
    temp3 = bignum_init()
    carry = 0
    factor = 1
    gtemp = 0
    gquot = 0
    grem = 0
    length = 0

    if (bignum_less(b1, b2)):
        quotient.length = 0
        bignum_copy(b1, remainder)

    elif (bignum_iszero(b1)):
        quotient.length = 0
        bignum_fromint(remainder, 0)

    elif (b2.length == 1):
        if (quotient.capacity < b1.length):
            quotient.capacity = b1.length

        for i in range(b1.length - 1, -1, -1):
            gtemp = carry * RADIX + b1.data[i]
            gquot = gtemp / b2.data[0]
            quotient.data[i] = gquot
            if (quotient.data[i] != 0 and length == 0):
                length = i + 1
            carry = gtemp % b2.data[0]

        bignum_fromint(remainder, carry)
        quotient.length = length

    else:
        n = b1.length + 1
        m = b2.length

        if (quotient.capacity < n - m):
            quotient.capacity = n - m

        bignum_copy(b1, b1copy)
        bignum_copy(b2, b2copy)

        while (b2copy.data[b2copy.length - 1] < HALFRADIX):
            factor *= 2
            bignum_imultiply(b2copy, NUMS[2])

        if (factor > 1):
            bignum_fromint(temp, factor)
            bignum_imultiply(b1copy, temp)

        if (b1copy.length != n):
            b1copy.length += 1
            if (b1copy.length > b1copy.capacity):
                b1copy.capacity = b1copy.length

            b1copy.data[n - 1] = 0

        for i in range(n - m - 1, -1, -1):
            gtemp = RADIX * b1copy.data[i + m] + b1copy.data[i + m - 1]
            gquot = gtemp / b2copy.data[m - 1]
            if (gquot >= RADIX):
                gquot = UINT_MAX

            grem = gtemp % b2copy.data[m - 1]
            while (grem < RADIX and gquot * b2copy.data[m - 2] > RADIX * grem + b1copy.data[i + m - 2]):
                gquot -= 1
                grem += b2copy.data[m - 1]

            quottemp.data[0] = gquot % RADIX
            quottemp.data[1] = (gquot / RADIX)
            if (quottemp.data[1] != 0):
                quottemp.length = 2
            else:
                quottemp.length = 1

            bignum_multiply(temp2, b2copy, quottemp)
            if (m + 1 > temp3.capacity):
                temp3.capacity = m + 1

            temp3.length = 0
            for j in range(0, m + 1):
                temp3.data[j] = b1copy.data[i + j]

                if (temp3.data[j] != 0):
                    temp3.length = j + 1

            if (bignum_less(temp3, temp2)):
                bignum_iadd(temp3, b2copy)
                gquot -= 1

            bignum_isubtract(temp3, temp2)

            for j in range(0, temp3.length):
                b1copy.data[i + j] = temp3.data[j]

            for j in range(temp3.length, m + 1):
                b1copy.data[i + j] = 0

            quotient.data[i] = gquot

        if (quotient.datq[i] != 0):
            quotient.length = i

        if (quotient.data[b1.length - b2.length] == 0):
            quotient.length = b1.length - b2.length
        else:
            quotient.length = b1.length - b2.length + 1

        carry = 0
        for i in range(b1copy.length - 1, -1, -1):
            gtemp = b1copy.data[i] + carry * RADIX
            b1copy.data[i] = gtemp / factor
            if (b1copy.data[i] != 0 and length == 0):
                b1copy.length = i + 1
            carry = gtemp % factor

        bignum_deinit(b2copy)
        bignum_deinit(b1copy)
        bignum_deinit(quottemp)
        bignum_deinit(temp)
        bignum_deinit(temp2)
        bignum_deinit(temp3)


def bignum_modpow(base, exponent, modulus, result):
    a = bignum_init()
    b = bignum_init()
    c = bignum_init()
    discard = bignum_init()
    remainder = bignum_init()

    bignum_copy(base, a)
    bignum_copy(exponent, b)
    bignum_copy(modulus, c)
    bignum_fromint(result, 1)

    while (bignum_greater(b, NUMS[0])):
        if (b.data[0] & 1):
            bignum_imultiply(result, a)
            bignum_imodulate(result, c)

        bignum_idivide(b, NUMS[2])
        bignum_copy(a, discard)
        bignum_imultiply(a, discard)
        bignum_imodulate(a, c)

    bignum_deinit(a)
    bignum_deinit(b)
    bignum_deinit(c)
    bignum_deinit(discard)
    bignum_deinit(remainder)


def bignum_gcd(b1, b2, result):
    a = bignum_init()
    b = bignum_init()
    remainder = bignum_init()
    temp = bignum_init()
    discard = bignum_init()

    bignum_copy(b1, a)
    bignum_copy(b2, b)

    while (not bignum_equal(b, NUMS[0])):
        bignum_copy(b, temp)
        bignum_imodulate(a, b)
        bignum_copy(a, b)
        bignum_copy(temp, a)

    bignum_copy(a, result)

    bignum_deinit(a)
    bignum_deinit(b)
    bignum_deinit(remainder)
    bignum_deinit(temp)
    bignum_deinit(discard)


def bignum_inverse(a, m, result):
    remprev = bignum_init()
    rem = bignum_init()
    auxprev = bignum_init()
    aux = bignum_init()
    rcur = bignum_init()
    qcur = bignum_init()
    acur = bignum_init()

    bignum_copy(m, remprev)
    bignum_copy(a, rem)
    bignum_fromint(auxprev, 0)
    bignum_fromint(aux, 1)
    while (bignum_greater(rem, NUMS[1])):
        bignum_divide(qcur, rcur, remprev, rem)
        bignum_subtract(acur, m, qcur)
        bignum_imultiply(acur, aux)
        bignum_iadd(acur, auxprev)
        bignum_imodulate(acur, m)

        bignum_copy(rem, remprev)
        bignum_copy(aux, auxprev)
        bignum_copy(rcur, rem)
        bignum_copy(acur, aux)

    bignum_copy(acur, result)

    bignum_deinit(remprev)
    bignum_deinit(rem)
    bignum_deinit(auxprev)
    bignum_deinit(aux)
    bignum_deinit(rcur)
    bignum_deinit(qcur)
    bignum_deinit(acur)


def bignum_jacobi(ac, nc):
    remainder = bignum_init()
    twos = bignum_init()
    temp = bignum_init()
    a = bignum_init()
    n = bignum_init()
    mult = 1
    result = 0

    bignum_copy(ac, a)
    bignum_copy(nc, n)

    while (bignum_greater(a, NUMS[1]) and not bignum_equal(a, n)):
        bignum_imodulate(a, n)
        if (bignum_leq(a, NUMS[1]) or bignum_equal(a, n)):
            break
        bignum_fromint(twos, 0)
        # Factoriser les multiples de deux
        while (a.data[0] % 2 == 0):
            bignum_iadd(twos, NUMS[1])
            bignum_idivide(a, NUMS[2])

        # Coeff
        if (bignum_greater(twos, NUMS[0]) and twos.data[0] % 2 == 1):
            bignum_remainder(n, NUMS[8], remainder)
            if (not bignum_equal(remainder, NUMS[1]) and not bignum_equal(remainder, NUMS[7])):
                mult *= -1

        if (bignum_leq(a, NUMS[1]) or bignum_equal(a, n)):
            break

        bignum_remainder(n, NUMS[4], remainder)
        bignum_remainder(a, NUMS[4], temp)
        if (not bignum_equal(remainder, NUMS[1]) and not bignum_equal(temp, NUMS[1])):
            mult *= -1

        bignum_copy(a, temp)
        bignum_copy(n, a)
        bignum_copy(temp, n)

    if (bignum_equal(a, NUMS[1])):
        result = mult
    else:
        result = 0

    bignum_deinit(remainder)
    bignum_deinit(twos)
    bignum_deinit(temp)
    bignum_deinit(a)
    bignum_deinit(n)

    return result


def solovayPrime(a, n):
    ab = bignum_init()
    res = bignum_init()
    pow = bignum_init()
    modpow = bignum_init()
    result = 0

    bignum_fromint(ab, a)
    x = bignum_jacobi(ab, n)
    if (x == -1):
        bignum_subtract(res, n, NUMS[1])
    else:
        bignum_fromint(res, x)
    bignum_copy(n, pow)
    bignum_isubtract(pow, NUMS[1])
    bignum_idivide(pow, NUMS[2])
    bignum_modpow(ab, pow, n, modpow)

    result = not bignum_equal(res, NUMS[0]) and bignum_equal(modpow, res)

    bignum_deinit(ab)
    bignum_deinit(res)
    bignum_deinit(pow)
    bignum_deinit(modpow)

    return result


def probablePrime(n, k):
    if (bignum_equal(n, NUMS[2])):
        return True
    elif (n.data[0] % 2 == 0 or bignum_equal(n, NUMS[1])):
        return False
    while (k > 0):
        k -= 1
        if (n.length <= 1):
            wit = random.randint(2, n.data[0] - 2)
            if (not solovayPrime(wit, n)):
                return False
        else:
            wit = random.randint(2, RAND_MAX - 2)
            if (not solovayPrime(wit, n)):
                return False
    return True


'''void randPrime(int numDigits, bignum *result)
{
    char *string = malloc((numDigits + 1) * sizeof(char));
    int i;
    string[0] = (rand() % 9) + '1';                 // Pas de zéros initiaux
    string[numDigits - 1] = (rand() % 5) * 2 + '1'; // Dernier chiffre impair
    for (i = 1; i < numDigits - 1; i++)
        string[i] = (rand() % 10) + '0';
    string[numDigits] = '\0';
    bignum_fromstring(result, string);
    while (1)
    {
        if (probablePrime(result, ACCURACY))
        {
            free(string);
            return;
        }
        bignum_iadd(result, &NUMS[2]); // result += 2
    }
}'''


def randPrime(numDigits, result):
    string = bytearray(numDigits + 1)
    string[0] = random.randint(1, 9) + ord('0')
    string[numDigits - 1] = random.randint(0, 4) * 2 + ord('1')
    for i in range(1, numDigits - 1):
        string[i] = random.randint(0, 9) + ord('0')
    string[numDigits] = ord('\0')
    bignum_fromstring(result, string)
    while 1:
        if (probablePrime(result, ACCURACY)):
            return
        bignum_iadd(result, NUMS[2])


def randExponent(phi, n, result):
    gcd = bignum_init()
    e = random.randint(0, n)

    while 1:
        bignum_fromint(result, e)
        bignum_gcd(result, phi, gcd)
        if (bignum_equal(gcd, NUMS[1])):
            bignum_deinit(gcd)
            return
        e = (e + 1) % n
        if (e <= 2):
            e = 3


def readFile(fd, bytes):
    len, cap = 0, BUF_SIZE
    buffer = bytearray(BUF_SIZE)
    while True:
        buf = fd.read(BUF_SIZE)
        r = len(buf)
        if not buf:
            break
        if len + r >= cap:
            cap *= 2
            buffer = bytearray(buffer)
            buffer += bytearray(cap - len)
        buffer[len:len+r] = buf
        len += r
    # Fill the last block with zeros to signal the end of the cryptogram.
    # An additional block is added if there is no room.
    if len + bytes - len % bytes > cap:
        buffer += bytearray(len + bytes - len % bytes - cap)
    while len % bytes != 0:
        buffer[len] = 0
        len += 1
    return buffer[:len]


def encode(m, e, n, result):
    bignum_modpow(m, e, n, result)


def decode(c, d, n, result):
    bignum_modpow(c, d, n, result)


def encodeMessage(len, bytes, message, exponent, modulus):
    encoded = []
    num128 = bignum_init()
    num128pow = bignum_init()
    x = bignum_init()
    current = bignum_init()
    bignum_fromint(num128, 128)
    bignum_fromint(num128pow, 1)
    for i in range(0, len, bytes):
        bignum_fromint(x, 0)
        bignum_fromint(num128pow, 1)
        for j in range(0, bytes):
            bignum_fromint(current, message[i + j])
            bignum_imultiply(current, num128pow)
            bignum_iadd(x, current)
            bignum_imultiply(num128pow, num128)
        encoded.append(encode(x, exponent, modulus))
    return encoded


'''int *decodeMessage(int len, int bytes, bignum *cryptogram, bignum *exponent, bignum *modulus)
{
    int *decoded = malloc(len * bytes * sizeof(int));
    int i, j;
    bignum *x = bignum_init(), *remainder = bignum_init();
    bignum *num128 = bignum_init();
    bignum_fromint(num128, 128);
    for (i = 0; i < len; i++)
    {
        decode(&cryptogram[i], exponent, modulus, x);
        for (j = 0; j < bytes; j++)
        {
            bignum_idivider(x, num128, remainder);
            if (remainder->length == 0)
                decoded[i * bytes + j] = (char)0;
            else
                decoded[i * bytes + j] = (char)(remainder->data[0]);
#ifndef NOPRINT
            printf("%c", (char)(decoded[i * bytes + j]));
#endif
        }
    }
    return decoded;
}
'''


def decodeMessage(len, bytes, cryptogram, exponent, modulus):
    decoded = []
    num128 = bignum_init()
    x = bignum_init()
    remainder = bignum_init()
    bignum_fromint(num128, 128)
    for i in range(len):
        decode(cryptogram[i], exponent, modulus, x)
        for j in range(bytes):
            bignum_idivide(x, num128, remainder)
            if (remainder.length == 0):
                decoded.append(0)
            else:
                decoded.append(remainder.data[0])
    return decoded


def main():
    p = bignum_init()
    q = bignum_init()
    n = bignum_init()
    phi = bignum_init()
    e = bignum_init()
    d = bignum_init()
    bbytes = bignum_init()
    shift = bignum_init()
    temp1 = bignum_init()
    temp2 = bignum_init()

    randPrime(FACTOR_DIGITS, p)
    print("Got first prime factor, p = ", end="")
    bignum_print(p)
    print(" ... ", end="")
    input()

    randPrime(FACTOR_DIGITS, q)
    print("Got second prime factor, q = ", end="")
    bignum_print(q)
    print(" ... ", end="")
    input()

    bignum_multiply(n, p, q)
    print("Got modulus, n = pq = ", end="")
    bignum_print(n)
    print(" ... ", end="")
    input()

    bignum_subtract(temp1, p, NUMS[1])
    bignum_subtract(temp2, q, NUMS[1])
    bignum_multiply(phi, temp1, temp2)  # phi = (p - 1) * (q - 1)
    print("Got totient, phi = ", end="")
    bignum_print(phi)
    print(" ... ", end="")
    input()

    randExponent(phi, EXPONENT_MAX, e)
    print("Chose public exponent, e = ", end="")
    bignum_print(e)
    print("\nPublic key is (", end="")
    bignum_print(e)
    print(", ", end="")
    bignum_print(n)
    print(") ... ", end="")
    input()

    bignum_inverse(e, phi, d)
    print("Calculated private exponent, d = ", end="")
    bignum_print(d)
    print("\nPrivate key is (", end="")
    bignum_print(d)
    print(", ", end="")
    bignum_print(n)
    print(") ... ", end="")
    input()

    # Calculer le nombre de caractères que l'on peut encoder dans un bloc de chiffrement
    bytes = -1
    bignum_fromint(shift, 1 << 7)  # 7 bits per char
    bignum_fromint(bbytes, 1)
    while bignum_less(bbytes, n):

        # Décaler d'un octet, NB: on utilise la représentation par masque binaire, donc on peut utiliser un décalage
        bignum_imultiply(bbytes, shift)
        bytes += 1

    print("Opening file \"message_M.txt\" for reading")
    f = open("message_M.txt", "r")
    if f is None:
        print("Failed to open file \"message_M.txt\". Does it exist?")
        return exit(1)
    len, buffer = readFile(f, bytes)

    print("File \"message_M.txt\" read successfully, %d bytes read. Encoding byte stream in chunks of %d bytes ... " % (
        len, bytes), end="")
    input()
    print()
    encoded = encodeMessage(len, bytes, buffer, e, n)
    print("\n\nEncoding finished successfully ... ", end="")
    input()

    print("Decoding encoded message ... ", end="")
    input()
    print()
    decoded = decodeMessage(len // bytes, bytes, encoded, d, n)
    print("\n\nFinished RSA demonstration!")

    bignum_deinit(p)
    bignum_deinit(q)
    bignum_deinit(n)
    bignum_deinit(phi)
    bignum_deinit(e)
    bignum_deinit(d)
    bignum_deinit(bbytes)
    bignum_deinit(shift)
    bignum_deinit(temp1)
    bignum_deinit(temp2)
    f.close()


if __name__ == "__main__":
    main()
