#ifndef BYTEORDER_H
#define BYTEORDER_H

/* The following macros are similar to [nh]to[hn][ls](), except that the
 * network-ordered integer is referred to by a pointer, and does not need to
 * be aligned.  This is very handy and efficient when reading protocol
 * headers, e.g.
 *   uint16_t sport = nptohs(&udp->th_sport);
 * Note that it's ok to take the ADDRESS of members of unaligned structures,
 * just never try to use the VALUE of the member.
 */

/* Convert the network order 32 bit integer pointed to by p to host order.
 * p does not have to be aligned. */
#define nptohl(p) \
   ((((uint8_t*)(p))[0] << 24) | \
    (((uint8_t*)(p))[1] << 16) | \
    (((uint8_t*)(p))[2] << 8) | \
    ((uint8_t*)(p))[3])

/* Convert the network order 16 bit integer pointed to by p to host order.
 * p does not have to be aligned. */
#define nptohs(p) \
   ((((uint8_t*)(p))[0] << 8) | ((uint8_t*)(p))[1])

/* Copy the host order 16 bit integer in x into the memory pointed to by p
 * in network order.  p does not have to be aligned. */
#define htonps(p, x) \
    do { \
        ((uint8_t*)(p))[0] = (x & 0xFF00) >> 8; \
        ((uint8_t*)(p))[1] = (x & 0x00FF) >> 0; \
    } while (0)

/* Copy the host order 32 bit integer in x into the memory pointed to by p
 * in network order.  p does not have to be aligned. */
#define htonpl(p, x) \
    do { \
        ((uint8_t*)(p))[0] = (x & 0xFF000000) >> 24; \
        ((uint8_t*)(p))[1] = (x & 0x00FF0000) >> 16; \
        ((uint8_t*)(p))[2] = (x & 0x0000FF00) >> 8; \
        ((uint8_t*)(p))[3] = (x & 0x000000FF) >> 0; \
    } while (0)

#endif /* BYTEORDER_H */
