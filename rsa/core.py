def assert_int(var: int, name: str) -> None:
    if isinstance(var, int):
        return

    raise TypeError("%s should be an integer, not %s" % (name, var.__class__))


def encrypt_int(message: int, ekey: int, n: int) -> int:
    """Encrypts a message using encryption key 'ekey', working modulo n"""

    assert_int(message, "message")
    assert_int(ekey, "ekey")
    assert_int(n, "n")

    if message < 0:
        raise ValueError("Only non-negative numbers are supported")

    if message > n:
        raise OverflowError(
            "The message %i is too long for n=%i" % (message, n))

    return pow(message, ekey, n)


def decrypt_int(cyphertext: int, dkey: int, n: int) -> int:
    """Decrypts a cypher text using the decryption key 'dkey', working modulo n"""

    assert_int(cyphertext, "cyphertext")
    assert_int(dkey, "dkey")
    assert_int(n, "n")

    message = pow(cyphertext, dkey, n)
    return message
