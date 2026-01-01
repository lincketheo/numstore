# Release Signing Keys

This file contains PGP public keys used to sign official NumStore releases. Users can verify the authenticity and integrity of releases by checking signatures against these keys.

## What is This For?

When we create official releases of NumStore, we sign the release artifacts (tarballs, binaries, etc.) with PGP/GPG keys to ensure:
- **Authenticity**: The release actually comes from the NumStore project
- **Integrity**: The release has not been tampered with during distribution

## Verifying Releases

To verify a release signature:

```bash
# Import the signing key from this file
gpg --import KEYS.md

# Verify the signature (example)
gpg --verify numstore-0.1.0.tar.gz.asc numstore-0.1.0.tar.gz
```

You should see output indicating a "Good signature" from one of the keys listed below.

## Current Signing Keys

### Theo Lincke (Primary Release Key)

**Note**: Key not yet generated. Follow instructions below to set up.

---

## For Maintainers: Setting Up Release Signing

If you are a maintainer who will be signing releases, you need to:

### 1. Generate a PGP Key (if you don't have one)

```bash
# Generate a new GPG key
gpg --full-generate-key

# Choose:
# - Key type: RSA and RSA
# - Key size: 4096 bits
# - Expiration: 2 years (recommended)
# - Real name: Your Name
# - Email: your-email@example.com
# - Comment: "NumStore Release Signing Key"
```

### 2. Export Your Public Key

```bash
# List your keys to find the key ID
gpg --list-keys

# Export the public key (replace KEY_ID with your actual key ID)
gpg --armor --export KEY_ID > my-public-key.asc
```

### 3. Add Your Key to This File

Add your exported public key to the "Current Signing Keys" section above, including:
- Your name and role
- Key fingerprint
- Creation date
- The full ASCII-armored public key

Example format:
```
### Your Name (Role)

**Fingerprint**: `XXXX XXXX XXXX XXXX XXXX  XXXX XXXX XXXX XXXX XXXX`
**Created**: 2025-12-31
**Expires**: 2027-12-31

```
-----BEGIN PGP PUBLIC KEY BLOCK-----

[Your public key content here]

-----END PGP PUBLIC KEY BLOCK-----
```
```

### 4. Upload to Key Servers (Optional but Recommended)

```bash
# Upload to a key server for easier verification
gpg --keyserver keys.openpgp.org --send-keys KEY_ID
gpg --keyserver keyserver.ubuntu.com --send-keys KEY_ID
```

### 5. Sign Releases

When creating a release:

```bash
# Sign a release tarball
gpg --armor --detach-sign numstore-0.1.0.tar.gz

# This creates numstore-0.1.0.tar.gz.asc
# Include both files in the release
```

## Key Management Best Practices

1. **Keep private keys secure**: Never share your private key
2. **Use a strong passphrase**: Protect your private key with a strong passphrase
3. **Set expiration dates**: Keys should expire after 1-2 years
4. **Backup your keys**: Keep secure backups of your private key
5. **Revoke compromised keys**: If your key is compromised, revoke it immediately
6. **Use subkeys**: Consider using separate subkeys for signing

## Key Revocation

If a key needs to be revoked (compromised, lost, etc.):

1. Generate a revocation certificate
2. Publish the revocation to key servers
3. Update this KEYS.md file to mark the key as revoked
4. Remove the key from the "Current Signing Keys" section
5. Add it to a "Revoked Keys" section for historical reference

---

## Current Status

⚠️ **Action Required**: No signing keys have been configured yet.

### Next Steps for Project Maintainer:

1. **Generate your GPG key** using the instructions above
2. **Export your public key** and add it to this file
3. **Upload to key servers** for wider distribution
4. **Set up CI/CD signing** if automating releases
5. **Document key fingerprint** in release announcements

Once set up, all official releases should be signed with one of the keys listed in this file.
