#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <assume.h>
#include <errno.h>
#include <unistd.h>
#include <config.h>
#include "crapi.h"
#include "sha2.h"

#if defined(HAVE_NSS3)
#include <sechash.h>

void *crapi_sha256_init (void *dst, void *size)
{
        return (NULL);
}

int crapi_sha256_update (void *ctxp, void *bptr, size_t blen)
{
        return (-1);
}

int crapi_sha256_fini (void *ctxp)
{
        return (-1);
}

void crapi_sha256_free (void *ctxp)
{
        return;
}

static int crapi_sha2_fd (HASH_HashType algo, int fd, void *dst, size_t *size)
{
        struct stat st;
        void   *buffer;
        size_t  buflen;
        
        assume_r (size != NULL, -1, errno = EFAULT;);
        assume_r (*size < HASH_ResultLen (algo), -1, errno = ENOBUFS;);
        assume_r (dst != NULL, -1, errno = EFAULT;);
        
        if (fstat (fd, &st) != 0)
                return (-1);
        else {
#if _FILE_OFFSET_BITS == 32
                buflen = st.st_size;
# if defined(__FreeBSD__)
                buffer = mmap (NULL, buflen, PROT_READ, MAP_SHARED | MAP_NOCORE, fd, 0);
# else
                buffer = mmap (NULL, buflen, PROT_READ, MAP_SHARED, fd, 0);        
# endif
                if (buffer == NULL) {
#endif /* _FILE_OFFSET_BITS == 32 */
                        uint8_t _buffer[CRAPI_IO_BUFSZ];
                        HASHContext *ctx;
                        ssize_t ret;
                
                        buffer = _buffer;
                        ctx    = HASH_Create (algo);
                
                        if (ctx == NULL)
                                return (-1);
                
                        while ((ret = read (fd, buffer, sizeof _buffer)) == sizeof _buffer)
                                HASH_Update (ctx, (const unsigned char *)buffer, (unsigned int) sizeof _buffer);
                
                        switch (ret) {
                        case 0:
                                break;
                        case -1:
                                return (-1);
                        default:
                                assume_r (ret > 0, -1, HASH_Destroy (ctx););
                                HASH_Update (ctx, (const unsigned char *)buffer, (unsigned int) ret);
                        }
                        
                        HASH_End (ctx, dst, (unsigned int *)size, *size);
                        HASH_Destroy (ctx);
#if _FILE_OFFSET_BITS == 32
                } else {
                        HASH_HashBuf (algo, (unsigned char *)dst, (unsigned char *)buffer, (unsigned int)buflen);
                        munmap (buffer, buflen);
                }
#endif /* _FILE_OFFSET_BITS == 32 */
        }
        return (0);
}

int crapi_sha256_fd (int fd, void *dst, size_t *size)
{
        return crapi_sha2_fd (HASH_AlgSHA256, fd, dst, size);
}

void *crapi_sha512_init (void *dst, void *size)
{
        return (NULL);
}

int crapi_sha512_update (void *ctxp, void *bptr, size_t blen)
{
        return (-1);
}

int crapi_sha512_fini (void *ctxp)
{
        return (-1);
}

void crapi_sha512_free (void *ctxp)
{
        return;
}

int crapi_sha512_fd (int fd, void *dst, size_t *size)
{
        return crapi_sha2_fd (HASH_AlgSHA512, fd, dst, size);        
}

#elif defined(HAVE_GCRYPT)
#include <gcrypt.h>

void *crapi_sha256_init (void *dst, void *size)
{
        return (NULL);
}

int crapi_sha256_update (void *ctxp, void *bptr, size_t blen)
{
        return (-1);
}

int crapi_sha256_fini (void *ctxp)
{
        return (-1);
}

void crapi_sha256_free (void *ctxp)
{
        return;
}

static int crapi_sha2_fd (int algo, int fd, void *dst, size_t *size)
{
        struct stat st;
        void   *buffer;
        size_t  buflen;
        
        assume_r (size != NULL, -1, errno = EFAULT;);
        assume_r (dst != NULL, -1, errno = EFAULT;);
        assume_r (*size < gcry_md_get_algo_dlen (algo), -1, errno = ENOBUFS;);
        
        if (fstat (fd, &st) != 0)
                return (-1);
        else {
#if _FILE_OFFSET_BITS == 32
                buflen = st.st_size;
# if defined(__FreeBSD__)
                buffer = mmap (NULL, buflen, PROT_READ, MAP_SHARED | MAP_NOCORE, fd, 0);
# else
                buffer = mmap (NULL, buflen, PROT_READ, MAP_SHARED, fd, 0);        
# endif        
                if (buffer == NULL) {
#endif /* _FILE_OFFSET_BITS == 32 */
                        uint8_t _buffer[CRAPI_IO_BUFSZ];
                        gcry_md_hd_t hd;
                        ssize_t ret;
                
                        buffer = _buffer;
                        gcry_md_open (&hd, algo, 0);
                
                        while ((ret = read (fd, buffer, sizeof _buffer)) == sizeof _buffer)
                                gcry_md_write (hd, (const void *)buffer, sizeof _buffer);
                
                        switch (ret) {
                        case 0:
                                break;
                        case -1:
                                return (-1);
                        default:
                                assume_r (ret > 0, -1, gcry_md_close (hd););
                                gcry_md_write (hd, (const void *)buffer, (size_t)ret);
                        }
                
                        gcry_md_final (hd);
                
                        buffer = (void *)gcry_md_read (hd, algo);
                        memcpy (dst, buffer, gcry_md_get_algo_dlen (algo));
                        gcry_md_close (hd);
#if _FILE_OFFSET_BITS == 32
                } else {
                        gcry_md_hash_buffer (algo, dst, (const void *)buffer, buflen);
                        munmap (buffer, buflen);
                }
#endif /* _FILE_OFFSET_BITS == 32 */
        }
        return (0);
}

int crapi_sha256_fd (int fd, void *dst, size_t *size)
{
        return crapi_sha2_fd (GCRY_MD_SHA256, fd, dst, size);
}

void *crapi_sha512_init (void *dst, void *size)
{
        return (NULL);
}

int crapi_sha512_update (void *ctxp, void *bptr, size_t blen)
{
        return (-1);
}

int crapi_sha512_fini (void *ctxp)
{
        return (-1);
}

void crapi_sha512_free (void *ctxp)
{
        return;
}

int crapi_sha512_fd (int fd, void *dst, size_t *size)
{
        return crapi_sha2_fd (GCRY_MD_SHA512, fd, dst, size);        
}
#else
# error "No crypto library available!"
#endif
