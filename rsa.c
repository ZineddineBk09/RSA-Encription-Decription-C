#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>

/* FR: Accurance avec laquelle nous testons pour les nombres premiers en utilisant l'algorithme de Solovay-Strassen.
 * 20 Tests devrait être suffisant pour la plupart des nombres premiers de taille moyenne
 */
#define ACCURACY 20

#define FACTOR_DIGITS 100
#define EXPONENT_MAX RAND_MAX
#define BUF_SIZE 1024

/* FR: Capacité initiale pour une structure bignum. Ils s'élargiront de manière flexible mais cela
 * devrait être raisonnablement élevé pour éviter des reallocs fréquents tôt */

#define BIGNUM_CAPACITY 20

/* Radix and halfradix. These should be changed if the limb/word type changes */
/* FR: Radix et demi-radix. Ceux-ci devraient être changés si le type de membre / mot change */
#define RADIX 4294967296UL
#define HALFRADIX 2147483648UL

#define MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * FR: Type de membre de base. Notez que certaines calculs dépendent de l'overflow non signé de ce type.
 * En conséquence, seuls les types non signés doivent être utilisés ici, et le RADIX, HALFRADIX ci-dessus devraient être
 * changé si nécessaire. Entier non signé devrait probablement être le type de mot le plus efficace, et ceci
 * est utilisé par GMP par exemple.
 */
typedef unsigned int word;

/**
 * FR: Structure pour représenter des entiers multiples de précision. Ceci est une base "mot" LSB
 * représentation. Dans ce cas, la base, le mot, est 2 ^ 32. La longueur est le nombre de mots
 * dans la représentation actuelle. La longueur ne devrait pas permettre des zéros de queue (Choses comme 000124). La capacité est le nombre de mots alloués pour les données de membre.
 */
typedef struct _bignum
{
    int length;
    int capacity;
    word *data;
} bignum;

/**
 * FR: Quelques déclarations de forward comme ceci a été demandé d'être un seul fichier.
 * Voir les fonctions spécifiques pour les explications.
 */
void bignum_iadd(bignum *source, bignum *add);
void bignum_add(bignum *result, bignum *b1, bignum *b2);
void bignum_isubtract(bignum *source, bignum *add);
void bignum_subtract(bignum *result, bignum *b1, bignum *b2);
void bignum_imultiply(bignum *source, bignum *add);
void bignum_multiply(bignum *result, bignum *b1, bignum *b2);
void bignum_idivide(bignum *source, bignum *div);
void bignum_idivider(bignum *source, bignum *div, bignum *remainder);
void bignum_remainder(bignum *source, bignum *div, bignum *remainder);
void bignum_imodulate(bignum *source, bignum *modulus);
void bignum_divide(bignum *quotient, bignum *remainder, bignum *b1, bignum *b2);

/**
 * FR: Sauvegarder quelques entiers multiples de précision fréquemment utilisés (0 - 10) afin qu'ils n'aient pas besoin d'être répétés
 */
word DATA0[1] = {0};
word DATA1[1] = {1};
word DATA2[1] = {2};
word DATA3[1] = {3};
word DATA4[1] = {4};
word DATA5[1] = {5};
word DATA6[1] = {6};
word DATA7[1] = {7};
word DATA8[1] = {8};
word DATA9[1] = {9};
word DATA10[1] = {10};
bignum NUMS[11] = {{1, 1, DATA0}, {1, 1, DATA1}, {1, 1, DATA2}, {1, 1, DATA3}, {1, 1, DATA4}, {1, 1, DATA5}, {1, 1, DATA6}, {1, 1, DATA7}, {1, 1, DATA8}, {1, 1, DATA9}, {1, 1, DATA10}};

/**
 * FR: Initialiser une structure bignum. C'est le seul moyen de créer en toute sécurité un bignum
 * et devrait être appelé où que l'un est déclaré. (Nous realloc la mémoire dans tous les autres cas
 * qui est techniquement sûr, mais peut causer des problèmes lorsque nous allons à libérer
 */
bignum *bignum_init()
{
    bignum *b = malloc(sizeof(bignum));
    b->length = 0;
    b->capacity = BIGNUM_CAPACITY;
    b->data = calloc(BIGNUM_CAPACITY, sizeof(word));
    return b;
}

/**
 * FR: Libérer les ressources utilisées par un bignum. Utiliser avec prudence pour éviter les fuites de mémoire.
 */
void bignum_deinit(bignum *b)
{
    free(b->data);
    free(b);
}

// Vérifier si le bignum donné est zéro
int bignum_iszero(bignum *b)
{
    return b->length == 0 || (b->length == 1 && b->data[0] == 0);
}

// Vérifier si le bignum donné est non zéro
int bignum_isnonzero(bignum *b)
{
    return !bignum_iszero(b);
}

// FR: Copier du bignum source dans le bignum de destination.
void bignum_copy(bignum *source, bignum *dest)
{
    dest->length = source->length;
    if (source->capacity > dest->capacity)
    {
        dest->capacity = source->capacity;
        dest->data = realloc(dest->data, dest->capacity * sizeof(word));
    }
    memcpy(dest->data, source->data, dest->length * sizeof(word));
}

// FR: Charger un bignum à partir d'une chaîne de base 10. Seules les chaînes numériques pures fonctionneront.
void bignum_fromstring(bignum *b, char *string)
{
    int i, len = 0;
    while (string[len] != '\0')
        len++; // Trouver la taille de string
    for (i = 0; i < len; i++)
    {
        if (i != 0)
            bignum_imultiply(b, &NUMS[10]); // FR: Multiplier par 10
        bignum_iadd(b, &NUMS[string[i] - '0']);
    }
}

/**
 * Load a bignum from an unsigned integer.
 */
// FR: Charger un bignum à partir d'un entier non signé.
void bignum_fromint(bignum *b, unsigned int num)
{
    b->length = 1;
    if (b->capacity < b->length)
    {
        b->capacity = b->length;
        b->data = realloc(b->data, b->capacity * sizeof(word));
    }
    b->data[0] = num;
}

/**
 * FR: Imprimer un bignum sur stdout comme entier de base 10. Ceci est fait par
 * division répétée par 10. Nous pouvons le rendre plus efficace en divisant par
 * 10^9 par exemple, puis en faisant de l'arithmétique à précision simple pour récupérer le
 * 9 restes
 */
void bignum_print(bignum *b)
{
    int cap = 100, len = 0, i;
    char *buffer = malloc(cap * sizeof(char));
    bignum *copy = bignum_init(), *remainder = bignum_init();
    if (b->length == 0 || bignum_iszero(b))
        printf("0");
    else
    {
        bignum_copy(b, copy);
        while (bignum_isnonzero(copy))
        {
            bignum_idivider(copy, &NUMS[10], remainder);
            buffer[len++] = remainder->data[0];
            if (len >= cap)
            {
                cap *= 2;
                buffer = realloc(buffer, cap * sizeof(char));
            }
        }
        for (i = len - 1; i >= 0; i--)
            printf("%d", buffer[i]);
    }
    bignum_deinit(copy);
    bignum_deinit(remainder);
    free(buffer);
}

// Vérifier si deux bignums sont égaux.
int bignum_equal(bignum *b1, bignum *b2)
{
    int i;
    if (bignum_iszero(b1) && bignum_iszero(b2))
        return 1;
    else if (bignum_iszero(b1))
        return 0;
    else if (bignum_iszero(b2))
        return 0;
    else if (b1->length != b2->length)
        return 0;
    for (i = b1->length - 1; i >= 0; i--)
    {
        if (b1->data[i] != b2->data[i])
            return 0;
    }
    return 1;
}

// Vérifier si le bignum b1 est supérieur au bignum b2
int bignum_greater(bignum *b1, bignum *b2)
{
    int i;
    if (bignum_iszero(b1) && bignum_iszero(b2))
        return 0;
    else if (bignum_iszero(b1))
        return 0;
    else if (bignum_iszero(b2))
        return 1;
    else if (b1->length != b2->length)
        return b1->length > b2->length;
    for (i = b1->length - 1; i >= 0; i--)
    {
        if (b1->data[i] != b2->data[i])
            return b1->data[i] > b2->data[i];
    }
    return 0;
}

// Vérifier si le bignum b1 est inférieur au bignum b2
int bignum_less(bignum *b1, bignum *b2)
{
    int i;
    if (bignum_iszero(b1) && bignum_iszero(b2))
        return 0;
    else if (bignum_iszero(b1))
        return 1;
    else if (bignum_iszero(b2))
        return 0;
    else if (b1->length != b2->length)
        return b1->length < b2->length;
    for (i = b1->length - 1; i >= 0; i--)
    {
        if (b1->data[i] != b2->data[i])
            return b1->data[i] < b2->data[i];
    }
    return 0;
}

// FR: Vérifier si le bignum b1 est supérieur ou égal au bignum b2
int bignum_geq(bignum *b1, bignum *b2)
{
    return !bignum_less(b1, b2);
}

// FR: Vérifier si le bignum b1 est inférieur ou égal au bignum b2
int bignum_leq(bignum *b1, bignum *b2)
{
    return !bignum_greater(b1, b2);
}

// FR: Effectuer une addition in place dans le bignum source. C'est à dire source += add
void bignum_iadd(bignum *source, bignum *add)
{
    bignum *temp = bignum_init();
    bignum_add(temp, source, add);
    bignum_copy(temp, source);
    bignum_deinit(temp);
}

// FR: Additionner deux bignums par la méthode add with carry. result = b1 + b2
void bignum_add(bignum *result, bignum *b1, bignum *b2)
{
    word sum, carry = 0;
    int i, n = MAX(b1->length, b2->length);
    if (n + 1 > result->capacity)
    {
        result->capacity = n + 1;
        result->data = realloc(result->data, result->capacity * sizeof(word));
    }
    for (i = 0; i < n; i++)
    {
        sum = carry;
        if (i < b1->length)
            sum += b1->data[i];
        if (i < b2->length)
            sum += b2->data[i];
        result->data[i] = sum; // FR: On a déjà pris le modulo 2^32 par le débordement de l'entier signé

        if (i < b1->length)
        {
            if (sum < b1->data[i])
                carry = 1; // FR: Si le résultat est inférieur à la valeur du premier bignum, alors il y a eu un débordement
            else
                carry = 0;
        }
        else
        {
            if (sum < b2->data[i])
                carry = 1; // FR: Si le résultat est inférieur à la valeur du deuxième bignum, alors il y a eu un débordement
            else
                carry = 0;
        }
    }
    if (carry == 1)
    {
        result->length = n + 1;
        result->data[n] = 1;
    }
    else
    {
        result->length = n;
    }
}

// Effectuer une soustraction in place dans le bignum source. C'est à dire source -= sub
void bignum_isubtract(bignum *source, bignum *sub)
{
    bignum *temp = bignum_init();
    bignum_subtract(temp, source, sub);
    bignum_copy(temp, source);
    bignum_deinit(temp);
}

/**
 * Soustraire le bignum b2 du bignum b1. result = b1 - b2. Le résultat est indéfini si b2 > b1.
 * Cette méthode utilise la méthode de soustraction avec retenue
 */
void bignum_subtract(bignum *result, bignum *b1, bignum *b2)
{
    int length = 0, i;
    word carry = 0, diff, temp;
    if (b1->length > result->capacity)
    {
        result->capacity = b1->length;
        result->data = realloc(result->data, result->capacity * sizeof(word));
    }
    for (i = 0; i < b1->length; i++)
    {
        temp = carry;
        if (i < b2->length)
            temp = temp + b2->data[i];
        diff = b1->data[i] - temp;
        if (temp > b1->data[i])
            carry = 1;
        else
        {
            if (temp == 0 && b2->data[i] == 0xffffffff)
            {
                carry = 1;
            }
            else
            {
                carry = 0;
            }
        }
        result->data[i] = diff;
        if (result->data[i] != 0)
            length = i + 1;
    }
    result->length = length;
}

// FR: Effectuer une multiplication in place dans le bignum source. C'est à dire source *= mult
void bignum_imultiply(bignum *source, bignum *mult)
{
    bignum *temp = bignum_init();
    bignum_multiply(temp, source, mult);
    bignum_copy(temp, source);
    bignum_deinit(temp);
}

/**
 * Multiplier deux bignums par la méthode de l'école. result = b1 * b2. J'ai expérimenté
 * avec la multiplication par FFT et Karatsuba mais aucune n'était plus efficace que la méthode
 * de l'école pour un nombre raisonnable de chiffres. Il y a des améliorations à faire ici,
 * notamment pour la multiplication par un carré qui peut couper la moitié des opérations.
 */
void bignum_multiply(bignum *result, bignum *b1, bignum *b2)
{
    int i, j, k;
    word carry, temp;
    unsigned long long int prod;
    if (b1->length + b2->length > result->capacity)
    {
        result->capacity = b1->length + b2->length;
        result->data = realloc(result->data, result->capacity * sizeof(word));
    }
    for (i = 0; i < b1->length + b2->length; i++)
        result->data[i] = 0;

    for (i = 0; i < b1->length; i++)
    {
        for (j = 0; j < b2->length; j++)
        {
            prod = (b1->data[i] * (unsigned long long int)b2->data[j]) + (unsigned long long int)(result->data[i + j]); /* This should not overflow... */
            carry = (word)(prod / RADIX);

            // FR: Ajouter la retenue au mot suivant, mais cela peut causer un débordement... propager
            k = 1;
            while (carry > 0)
            {
                temp = result->data[i + j + k] + carry;
                if (temp < result->data[i + j + k])
                    carry = 1;
                else
                    carry = 0;
                result->data[i + j + k] = temp;
                k++;
            }

            prod = (result->data[i + j] + b1->data[i] * (unsigned long long int)b2->data[j]) % RADIX; /* Again, should not overflow... */
            result->data[i + j] = prod;                                                               /* Add */
        }
    }
    if (b1->length + b2->length > 0 && result->data[b1->length + b2->length - 1] == 0)
        result->length = b1->length + b2->length - 1;
    else
        result->length = b1->length + b2->length;
}

// FR: Effectuer une division in place dans le bignum source. C'est à dire source /= div
void bignum_idivide(bignum *source, bignum *div)
{
    bignum *q = bignum_init(), *r = bignum_init();
    bignum_divide(q, r, source, div);
    bignum_copy(q, source);
    bignum_deinit(q);
    bignum_deinit(r);
}

/**
 * Effectuer une division in place dans le bignum source. C'est à dire source /= div
 * et remainder = source - source/div.
 */
void bignum_idivider(bignum *source, bignum *div, bignum *remainder)
{
    bignum *q = bignum_init(), *r = bignum_init();
    bignum_divide(q, r, source, div);
    bignum_copy(q, source);
    bignum_copy(r, remainder);
    bignum_deinit(q);
    bignum_deinit(r);
}

// FR: Calculer le reste de la division de source par div.
void bignum_remainder(bignum *source, bignum *div, bignum *remainder)
{
    bignum *q = bignum_init();
    bignum_divide(q, remainder, source, div);
    bignum_deinit(q);
}

// FR: Moduler le source par le modulus. source = source % modulus
void bignum_imodulate(bignum *source, bignum *modulus)
{
    bignum *q = bignum_init(), *r = bignum_init();
    bignum_divide(q, r, source, modulus);
    bignum_copy(r, source);
    bignum_deinit(q);
    bignum_deinit(r);
}

/**
 * Diviser deux bignums par la méthode de la division longue, produisant un quotient et un reste.
 * quotient = floor(b1/b2), remainder = b1 - quotient * b2. Si b1 < b2 le quotient est
 * triviallement 0 et le reste est b2.
 */
void bignum_divide(bignum *quotient, bignum *remainder, bignum *b1, bignum *b2)
{
    bignum *b2copy = bignum_init(), *b1copy = bignum_init();
    bignum *temp = bignum_init(), *temp2 = bignum_init(), *temp3 = bignum_init();
    bignum *quottemp = bignum_init();
    word carry = 0;
    int n, m, i, j, length = 0;
    unsigned long long factor = 1;
    unsigned long long gquot, gtemp, grem;
    if (bignum_less(b1, b2))
    {
        // Cas trivial, b1/b2 = 0 si b1 < b2
        quotient->length = 0;
        bignum_copy(b1, remainder);
    }
    else if (bignum_iszero(b1))
    {
        // FR: Si b1 est nul, le quotient est nul et le reste est nul.
        quotient->length = 0;
        bignum_fromint(remainder, 0);
    }
    else if (b2->length == 1)
    {
        // Division par un seul mot, on peut faire une division simple.
        if (quotient->capacity < b1->length)
        {
            quotient->capacity = b1->length;
            quotient->data = realloc(quotient->data, quotient->capacity * sizeof(word));
        }
        for (i = b1->length - 1; i >= 0; i--)
        {
            gtemp = carry * RADIX + b1->data[i];
            gquot = gtemp / b2->data[0];
            quotient->data[i] = gquot;
            if (quotient->data[i] != 0 && length == 0)
                length = i + 1;
            carry = gtemp % b2->data[0];
        }
        bignum_fromint(remainder, carry);
        quotient->length = length;
    }
    else
    {
        n = b1->length + 1;
        m = b2->length;
        if (quotient->capacity < n - m)
        {
            quotient->capacity = n - m;
            quotient->data = realloc(quotient->data, (n - m) * sizeof(word));
        }
        bignum_copy(b1, b1copy);
        bignum_copy(b2, b2copy);
        /**
         * Normaliser.. multiplier le diviseur par 2 jusqu'à ce que le MSB >= HALFRADIX. Cela
         * assure une convergence rapide lors de l'estimation du quotient ci-dessous. On multiplie
         * aussi le dividende par le même facteur pour s'assurer que le résultat ne change pas.
         */
        while (b2copy->data[b2copy->length - 1] < HALFRADIX)
        {
            factor *= 2;
            bignum_imultiply(b2copy, &NUMS[2]);
        }
        if (factor > 1)
        {
            bignum_fromint(temp, factor);
            bignum_imultiply(b1copy, temp);
        }
        /**
         * S'assurer que le dividende est plus long que le diviseur original (pré-normalisé). Si ce n'est pas
         * le cas, on introduit un mot nul pour artificiellement l'agrandir.
         */
        if (b1copy->length != n)
        {
            b1copy->length++;
            if (b1copy->length > b1copy->capacity)
            {
                b1copy->capacity = b1copy->length;
                b1copy->data = realloc(b1copy->data, b1copy->capacity * sizeof(word));
            }
            b1copy->data[n - 1] = 0;
        }

        // FR: Calculer le quotient par division longue.
        for (i = n - m - 1; i >= 0; i--)
        {
            gtemp = RADIX * b1copy->data[i + m] + b1copy->data[i + m - 1];
            gquot = gtemp / b2copy->data[m - 1];
            if (gquot >= RADIX)
                gquot = UINT_MAX;
            grem = gtemp % b2copy->data[m - 1];
            while (grem < RADIX && gquot * b2copy->data[m - 2] > RADIX * grem + b1copy->data[i + m - 2])
            {
                gquot--;
                grem += b2copy->data[m - 1];
            }
            quottemp->data[0] = gquot % RADIX;
            quottemp->data[1] = (gquot / RADIX);
            if (quottemp->data[1] != 0)
                quottemp->length = 2;
            else
                quottemp->length = 1;
            bignum_multiply(temp2, b2copy, quottemp);
            if (m + 1 > temp3->capacity)
            {
                temp3->capacity = m + 1;
                temp3->data = realloc(temp3->data, temp3->capacity * sizeof(word));
            }
            temp3->length = 0;
            for (j = 0; j <= m; j++)
            {
                temp3->data[j] = b1copy->data[i + j];
                if (temp3->data[j] != 0)
                    temp3->length = j + 1;
            }
            if (bignum_less(temp3, temp2))
            {
                bignum_iadd(temp3, b2copy);
                gquot--;
            }
            bignum_isubtract(temp3, temp2);
            for (j = 0; j < temp3->length; j++)
                b1copy->data[i + j] = temp3->data[j];
            for (j = temp3->length; j <= m; j++)
                b1copy->data[i + j] = 0;
            quotient->data[i] = gquot;
            if (quotient->data[i] != 0)
                quotient->length = i;
        }

        if (quotient->data[b1->length - b2->length] == 0)
            quotient->length = b1->length - b2->length;
        else
            quotient->length = b1->length - b2->length + 1;

        // Diviser par le facteur maintenant pour trouver le reste final
        carry = 0;
        for (i = b1copy->length - 1; i >= 0; i--)
        {
            gtemp = carry * RADIX + b1copy->data[i];
            b1copy->data[i] = gtemp / factor;
            if (b1copy->data[i] != 0 && length == 0)
                length = i + 1;
            carry = gtemp % factor;
        }
        b1copy->length = length;
        bignum_copy(b1copy, remainder);
    }
    bignum_deinit(temp);
    bignum_deinit(temp2);
    bignum_deinit(temp3);
    bignum_deinit(b1copy);
    bignum_deinit(b2copy);
    bignum_deinit(quottemp);
}

/**
 * Effectuer l'exponentiation modulaire par répétition de la puissance carrée. Cela calculera
 * result = base^exponent mod modulus
 */
void bignum_modpow(bignum *base, bignum *exponent, bignum *modulus, bignum *result)
{
    bignum *a = bignum_init(), *b = bignum_init(), *c = bignum_init();
    bignum *discard = bignum_init(), *remainder = bignum_init();
    bignum_copy(base, a);
    bignum_copy(exponent, b);
    bignum_copy(modulus, c);
    bignum_fromint(result, 1);
    while (bignum_greater(b, &NUMS[0]))
    {
        if (b->data[0] & 1)
        {
            bignum_imultiply(result, a);
            bignum_imodulate(result, c);
        }
        bignum_idivide(b, &NUMS[2]);
        bignum_copy(a, discard);
        bignum_imultiply(a, discard);
        bignum_imodulate(a, c);
    }
    bignum_deinit(a);
    bignum_deinit(b);
    bignum_deinit(c);
    bignum_deinit(discard);
    bignum_deinit(remainder);
}

// Calculer le pgcd de deux bignums. result = pgcd (b1, b2)
void bignum_gcd(bignum *b1, bignum *b2, bignum *result)
{
    bignum *a = bignum_init(), *b = bignum_init(), *remainder = bignum_init();
    bignum *temp = bignum_init(), *discard = bignum_init();
    bignum_copy(b1, a);
    bignum_copy(b2, b);
    while (!bignum_equal(b, &NUMS[0]))
    {
        bignum_copy(b, temp);
        bignum_imodulate(a, b);
        bignum_copy(a, b);
        bignum_copy(temp, a);
    }
    bignum_copy(a, result);
    bignum_deinit(a);
    bignum_deinit(b);
    bignum_deinit(remainder);
    bignum_deinit(temp);
    bignum_deinit(discard);
}

// Calculer l'inverse de a mod m. Ou, result = a^-1 mod m.
void bignum_inverse(bignum *a, bignum *m, bignum *result)
{
    bignum *remprev = bignum_init(), *rem = bignum_init();
    bignum *auxprev = bignum_init(), *aux = bignum_init();
    bignum *rcur = bignum_init(), *qcur = bignum_init(), *acur = bignum_init();

    bignum_copy(m, remprev);
    bignum_copy(a, rem);
    bignum_fromint(auxprev, 0);
    bignum_fromint(aux, 1);
    while (bignum_greater(rem, &NUMS[1]))
    {
        bignum_divide(qcur, rcur, remprev, rem);
        /*
    Observation: on trouve l'inverse dans un corps fini, donc on peut utiliser
    un algorithme modifié qui évite les nombres négatifs ici
*/
        bignum_subtract(acur, m, qcur);
        bignum_imultiply(acur, aux);
        bignum_iadd(acur, auxprev);
        bignum_imodulate(acur, m);

        bignum_copy(rem, remprev);
        bignum_copy(aux, auxprev);
        bignum_copy(rcur, rem);
        bignum_copy(acur, aux);
    }

    bignum_copy(acur, result);

    bignum_deinit(remprev);
    bignum_deinit(rem);
    bignum_deinit(auxprev);
    bignum_deinit(aux);
    bignum_deinit(rcur);
    bignum_deinit(qcur);
    bignum_deinit(acur);
}

// Calculer le symbole de Jacobi, J(ac, nc).
int bignum_jacobi(bignum *ac, bignum *nc)
{
    bignum *remainder = bignum_init(), *twos = bignum_init();
    bignum *temp = bignum_init(), *a = bignum_init(), *n = bignum_init();
    int mult = 1, result = 0;
    bignum_copy(ac, a);
    bignum_copy(nc, n);
    while (bignum_greater(a, &NUMS[1]) && !bignum_equal(a, n))
    {
        bignum_imodulate(a, n);
        if (bignum_leq(a, &NUMS[1]) || bignum_equal(a, n))
            break;
        bignum_fromint(twos, 0);
        // Factoriser les multiples de deux
        while (a->data[0] % 2 == 0)
        {
            bignum_iadd(twos, &NUMS[1]);
            bignum_idivide(a, &NUMS[2]);
        }
        // Coeff
        if (bignum_greater(twos, &NUMS[0]) && twos->data[0] % 2 == 1)
        {
            bignum_remainder(n, &NUMS[8], remainder);
            if (!bignum_equal(remainder, &NUMS[1]) && !bignum_equal(remainder, &NUMS[7]))
            {
                mult *= -1;
            }
        }
        if (bignum_leq(a, &NUMS[1]) || bignum_equal(a, n))
            break;
        bignum_remainder(n, &NUMS[4], remainder);
        bignum_remainder(a, &NUMS[4], temp);
        if (!bignum_equal(remainder, &NUMS[1]) && !bignum_equal(temp, &NUMS[1]))
            mult *= -1;
        bignum_copy(a, temp);
        bignum_copy(n, a);
        bignum_copy(temp, n);
    }
    if (bignum_equal(a, &NUMS[1]))
        result = mult;
    else
        result = 0;
    bignum_deinit(remainder);
    bignum_deinit(twos);
    bignum_deinit(temp);
    bignum_deinit(a);
    bignum_deinit(n);
    return result;
}

// Vérifier si a est un témoin de Euler pour n. C'est-à-dire, si a^(n - 1)/2 != Ja(a, n) mod n
int solovayPrime(int a, bignum *n)
{
    bignum *ab = bignum_init(), *res = bignum_init(), *pow = bignum_init();
    bignum *modpow = bignum_init();
    int x, result;

    bignum_fromint(ab, a);
    x = bignum_jacobi(ab, n);
    if (x == -1)
        bignum_subtract(res, n, &NUMS[1]);
    else
        bignum_fromint(res, x);
    bignum_copy(n, pow);
    bignum_isubtract(pow, &NUMS[1]);
    bignum_idivide(pow, &NUMS[2]);
    bignum_modpow(ab, pow, n, modpow);

    result = !bignum_equal(res, &NUMS[0]) && bignum_equal(modpow, res);
    bignum_deinit(ab);
    bignum_deinit(res);
    bignum_deinit(pow);
    bignum_deinit(modpow);
    return result;
}

// Vérifier si n est probablement premier, en utilisant le test de Solovay-Strassen.
int probablePrime(bignum *n, int k)
{
    if (bignum_equal(n, &NUMS[2]))
        return 1;
    else if (n->data[0] % 2 == 0 || bignum_equal(n, &NUMS[1]))
        return 0;
    while (k-- > 0)
    {
        if (n->length <= 1)
        { /* Prevent a > n */
            if (!solovayPrime(rand() % (n->data[0] - 2) + 2, n))
                return 0;
        }
        else
        {
            int wit = rand() % (RAND_MAX - 2) + 2;
            if (!solovayPrime(wit, n))
                return 0;
        }
    }
    return 1;
}

/**
 * Générer un nombre premier aléatoire, avec un nombre spécifié de chiffres.
 * Cela générera une chaîne de chiffres de base 10 de longueur donnée, la convertira
 * en un bignum et puis fera une recherche croissante pour le premier nombre premier probable.
 */
void randPrime(int numDigits, bignum *result)
{
    char *string = malloc((numDigits + 1) * sizeof(char));
    int i;
    string[0] = (rand() % 9) + '1';                 // Fr: Pas de zéros initiaux
    string[numDigits - 1] = (rand() % 5) * 2 + '1'; // Fr: Dernier chiffre impair
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
}

/**
 * Choisir un exposant de clé publique aléatoire pour l'algorithme RSA. L'exposant sera
 * inférieur au module, n, et coprime à phi.
 */
void randExponent(bignum *phi, int n, bignum *result)
{
    bignum *gcd = bignum_init();
    int e = rand() % n;
    while (1)
    {
        bignum_fromint(result, e);
        bignum_gcd(result, phi, gcd);
        if (bignum_equal(gcd, &NUMS[1]))
        {
            bignum_deinit(gcd);
            return;
        }
        e = (e + 1) % n;
        if (e <= 2)
            e = 3;
    }
}

/**
 * Lire le fichier fd dans un tableau d'octets prêt pour le chiffrement.
 * Le tableau sera rempli de zéros jusqu'à ce qu'il divise le nombre d'octets
 * chiffrés par bloc. Retourne le nombre d'octets lus.
 */
int readFile(FILE *fd, char **buffer, int bytes)
{
    int len = 0, cap = BUF_SIZE, r;
    char buf[BUF_SIZE];
    *buffer = malloc(BUF_SIZE * sizeof(char));
    while ((r = fread(buf, sizeof(char), BUF_SIZE, fd)) > 0)
    {
        if (len + r >= cap)
        {
            cap *= 2;
            *buffer = realloc(*buffer, cap);
        }
        memcpy(&(*buffer)[len], buf, r);
        len += r;
    }
    /*
        Remplir le dernier bloc avec des zéros pour signaler la fin du cryptogramme. Un bloc supplémentaire est ajouté s'il n'y a pas de place
    */
    if (len + bytes - len % bytes > cap)
        *buffer = realloc(*buffer, len + bytes - len % bytes);
    do
    {
        (*buffer)[len] = '\0';
        len++;
    } while (len % bytes != 0);
    return len;
}

// Fr: Chiffrer le message m en utilisant l'exposant public et le module, résultat = m^e mod n
void encode(bignum *m, bignum *e, bignum *n, bignum *result)
{
    bignum_modpow(m, e, n, result);
}

// Fr: Déchiffrer le cryptogramme c en utilisant l'exposant privé et le module, résultat = c^d mod n
void decode(bignum *c, bignum *d, bignum *n, bignum *result)
{
    bignum_modpow(c, d, n, result);
}

/**
 * Encode the message of given length, using the public key (exponent, modulus)
 * The resulting array will be of size len/bytes, each index being the encryption
 * of "bytes" consecutive characters, given by m = (m1 + m2*128 + m3*128^2 + ..),
 * encoded = m^exponent mod modulus
 */
bignum *encodeMessage(int len, int bytes, char *message, bignum *exponent, bignum *modulus)
{
    /* Calloc works here because capacity = 0 forces a realloc by callees but we should really
     * bignum_init() all of these */
    int i, j;
    bignum *encoded = calloc(len / bytes, sizeof(bignum));
    bignum *num128 = bignum_init(), *num128pow = bignum_init();
    bignum *x = bignum_init(), *current = bignum_init();
    bignum_fromint(num128, 128);
    bignum_fromint(num128pow, 1);
    for (i = 0; i < len; i += bytes)
    {
        bignum_fromint(x, 0);
        bignum_fromint(num128pow, 1);
        /* Compute buffer[0] + buffer[1]*128 + buffer[2]*128^2 etc (base 128 representation for characters->int encoding)*/
        for (j = 0; j < bytes; j++)
        {
            bignum_fromint(current, message[i + j]);
            bignum_imultiply(current, num128pow);
            bignum_iadd(x, current); /*x += buffer[i + j] * (1 << (7 * j)) */
            bignum_imultiply(num128pow, num128);
        }
        encode(x, exponent, modulus, &encoded[i / bytes]);
#ifndef NOPRINT
        bignum_print(&encoded[i / bytes]);
        printf(" ");
#endif
    }
    return encoded;
}

/**
 * Decode the cryptogram of given length, using the private key (exponent, modulus)
 * Each encrypted packet should represent "bytes" characters as per encodeMessage.
 * The returned message will be of size len * bytes.
 */
int *decodeMessage(int len, int bytes, bignum *cryptogram, bignum *exponent, bignum *modulus)
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

/**
 * Main method to demostrate the system. Sets up primes p, q, and proceeds to encode and
 * decode the message given in "text.txt"
 */
int main(void)
{
    int i, bytes, len;
    bignum *p = bignum_init(), *q = bignum_init(), *n = bignum_init();
    bignum *phi = bignum_init(), *e = bignum_init(), *d = bignum_init();
    bignum *bbytes = bignum_init(), *shift = bignum_init();
    bignum *temp1 = bignum_init(), *temp2 = bignum_init();

    bignum *encoded;
    int *decoded;
    char *buffer;
    FILE *f;

    srand(time(NULL));

    randPrime(FACTOR_DIGITS, p);
    printf("Got first prime factor, p = ");
    bignum_print(p);
    printf(" ... ");
    getchar();

    randPrime(FACTOR_DIGITS, q);
    printf("Got second prime factor, q = ");
    bignum_print(q);
    printf(" ... ");
    getchar();

    bignum_multiply(n, p, q);
    printf("Got modulus, n = pq = ");
    bignum_print(n);
    printf(" ... ");
    getchar();

    bignum_subtract(temp1, p, &NUMS[1]);
    bignum_subtract(temp2, q, &NUMS[1]);
    bignum_multiply(phi, temp1, temp2); /* phi = (p - 1) * (q - 1) */
    printf("Got totient, phi = ");
    bignum_print(phi);
    printf(" ... ");
    getchar();

    randExponent(phi, EXPONENT_MAX, e);
    printf("Chose public exponent, e = ");
    bignum_print(e);
    printf("\nPublic key is (");
    bignum_print(e);
    printf(", ");
    bignum_print(n);
    printf(") ... ");
    getchar();

    bignum_inverse(e, phi, d);
    printf("Calculated private exponent, d = ");
    bignum_print(d);
    printf("\nPrivate key is (");
    bignum_print(d);
    printf(", ");
    bignum_print(n);
    printf(") ... ");
    getchar();

    /* Compute maximum number of bytes that can be encoded in one encryption */
    bytes = -1;
    bignum_fromint(shift, 1 << 7); /* 7 bits per char */
    bignum_fromint(bbytes, 1);
    while (bignum_less(bbytes, n))
    {
        bignum_imultiply(bbytes, shift); /* Shift by one byte, NB: we use bitmask representative so this can actually be a shift... */
        bytes++;
    }

    printf("Opening file \"text.txt\" for reading\n");
    f = fopen("text.txt", "r");
    if (f == NULL)
    {
        printf("Failed to open file \"text.txt\". Does it exist?\n");
        return EXIT_FAILURE;
    }
    len = readFile(f, &buffer, bytes); /* len will be a multiple of bytes, to send whole chunks */

    printf("File \"text.txt\" read successfully, %d bytes read. Encoding byte stream in chunks of %d bytes ... ", len, bytes);
    getchar();
    printf("\n");
    encoded = encodeMessage(len, bytes, buffer, e, n);
    printf("\n\nEncoding finished successfully ... ");
    getchar();

    printf("Decoding encoded message ... ");
    getchar();
    printf("\n");
    decoded = decodeMessage(len / bytes, bytes, encoded, d, n);
    printf("\n\nFinished RSA demonstration!");

    /* Eek! This is why we shouldn't of calloc'd those! */
    for (i = 0; i < len / bytes; i++)
        free(encoded[i].data);
    free(encoded);
    free(decoded);
    free(buffer);
    bignum_deinit(p);
    bignum_deinit(q);
    bignum_deinit(n);
    bignum_deinit(phi);
    bignum_deinit(e);
    bignum_deinit(d);
    bignum_deinit(bbytes);
    bignum_deinit(shift);
    bignum_deinit(temp1);
    bignum_deinit(temp2);
    fclose(f);

    return EXIT_SUCCESS;
}

// now let's analyse the complexity of the algorithm in space and time

// we have 3 functions, each of which has a complexity of O(n^2)
// so the total complexity is O(n^2) + O(n^2) + O(n^2) = O(n^2), where n is the length of the input

// the space complexity is O(n) + O(n) + O(n) = O(n)
// also the time complexity depends on the length of our key, which is O(n^2)
// so the total time complexity is O(n^2) + O(n^2) + O(n^2) = O(n^2)

// if you disagree with my analysis, please let me know in the comments or contact me on my website or email