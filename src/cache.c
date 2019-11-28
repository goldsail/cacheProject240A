//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include <stdio.h>
#include "cache.h"

const char *studentName = "Fan Jin";
const char *studentID   = "A53308732";
const char *email       = "f1jin@eng.ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//

typedef struct {
    uint32_t tag;
    uint8_t lru;
} CacheLine;

typedef struct {
    CacheLine *lines;
} CacheSet;

typedef struct {
    CacheSet *sets;
} Cache;

static Cache iCache;
static Cache dCache;
static Cache l2Cache;

static uint32_t blockBits;
static uint32_t iBits;
static uint32_t dBits;
static uint32_t l2Bits;

//------------------------------------//
//          Cache Functions           //
//------------------------------------//

// Initialize the Cache Hierarchy
//
void
init_cache()
{
    // Initialize cache stats
    icacheRefs        = 0;
    icacheMisses      = 0;
    icachePenalties   = 0;
    dcacheRefs        = 0;
    dcacheMisses      = 0;
    dcachePenalties   = 0;
    l2cacheRefs       = 0;
    l2cacheMisses     = 0;
    l2cachePenalties  = 0;

    blockBits = 0;
    while (blocksize > (1u << blockBits)) blockBits++;
  
    if (icacheSets > 0) {
        iBits = 0;
        while (icacheSets > (1u << iBits)) iBits++;
        iCache.sets = (CacheSet*)malloc(sizeof(CacheSet) * icacheSets);
        for (uint32_t i = 0; i < icacheSets; i++) {
            iCache.sets[i].lines = (CacheLine*)malloc(sizeof(CacheLine) * icacheAssoc);
            for (uint32_t j = 0; j < icacheAssoc; j++) {
                iCache.sets[i].lines[j].lru = icacheAssoc;
            }
        }
    }

    if (dcacheSets > 0) {
        dBits = 0;
        while (dcacheSets > (1u << dBits)) dBits++;
        dCache.sets = (CacheSet*)malloc(sizeof(CacheSet) * dcacheSets);
        for (uint32_t i = 0; i < dcacheSets; i++) {
            dCache.sets[i].lines = (CacheLine*)malloc(sizeof(CacheLine) * dcacheAssoc);
            for (uint32_t j = 0; j < dcacheAssoc; j++) {
                dCache.sets[i].lines[j].lru = dcacheAssoc;
            }
        }
    }

    if (l2cacheSets > 0) {
        l2Bits = 0;
        while (l2cacheSets > (1u << l2Bits)) l2Bits++;
        l2Cache.sets = (CacheSet*)malloc(sizeof(CacheSet) * l2cacheSets);
        for (uint32_t i = 0; i < l2cacheSets; i++) {
            l2Cache.sets[i].lines = (CacheLine*)malloc(sizeof(CacheLine) * l2cacheAssoc);
            for (uint32_t j = 0; j < l2cacheAssoc; j++) {
                l2Cache.sets[i].lines[j].lru = l2cacheAssoc;
            }
        }
    }
}

// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
icache_access(uint32_t addr)
{
    if (icacheSets == 0) {
        return l2cache_access(addr);
    }
    icacheRefs++;
    uint32_t tag = addr >> (iBits + blockBits);
    uint32_t loc = (addr >> blockBits) & (icacheSets - 1u);
    CacheLine *lines = iCache.sets[loc].lines;

    for (uint32_t i = 0; i < icacheAssoc; i++) {
        if (lines[i].lru < icacheAssoc && lines[i].tag == tag) {
            // cache hit
            for (uint32_t j = 0; j < icacheAssoc; j++) {
                if (lines[j].lru < lines[i].lru) {
                    lines[j].lru++;
                }
            }
            lines[i].lru = 0;
            return icacheHitTime;
        }
    }

    // cache miss
    uint32_t penalty = l2cache_access(addr);

    uint8_t pos = 0; // position to replace on cache miss
    for (uint32_t i = 0; i < icacheAssoc; i++) {
        if (lines[i].lru > lines[pos].lru) {
            pos = i;
        }
    }
    for (uint32_t j = 0; j < icacheAssoc; j++) {
        if (lines[j].lru < lines[pos].lru) {
            lines[j].lru++;
        }
    }
    lines[pos].lru = 0;
    lines[pos].tag = tag;

    icacheMisses++;
    icachePenalties += penalty;
    return icacheHitTime + penalty;
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
dcache_access(uint32_t addr)
{
    if (dcacheSets == 0) {
        return l2cache_access(addr);
    }
    dcacheRefs++;
    uint32_t tag = addr >> (dBits + blockBits);
    uint32_t loc = (addr >> blockBits) & (dcacheSets - 1u);
    CacheLine *lines = dCache.sets[loc].lines;
    for (uint32_t i = 0; i < dcacheAssoc; i++) {
        if (lines[i].lru < dcacheAssoc && lines[i].tag == tag) {
            // cache hit
            for (uint32_t j = 0; j < dcacheAssoc; j++) {
                if (lines[j].lru < lines[i].lru) {
                    lines[j].lru++;
                }
            }
            lines[i].lru = 0;
            return dcacheHitTime;
        }
    }

    // cache miss
    uint32_t penalty = l2cache_access(addr);

    uint8_t pos = 0; // position to replace on cache miss
    for (uint32_t i = 0; i < dcacheAssoc; i++) {
        if (lines[i].lru > lines[pos].lru) {
            pos = i;
        }
    }
    for (uint32_t j = 0; j < dcacheAssoc; j++) {
        if (lines[j].lru < lines[pos].lru) {
            lines[j].lru++;
        }
    }
    lines[pos].lru = 0;
    lines[pos].tag = tag;

    dcacheMisses++;
    dcachePenalties += penalty;
    return dcacheHitTime + penalty;
}

void icache_invalid(uint32_t addr) {
    if (icacheSets == 0) {
        return;
    }
    uint32_t tag = addr >> (iBits + blockBits);
    uint32_t loc = (addr >> blockBits) & (icacheSets - 1u);
    CacheLine *lines = iCache.sets[loc].lines;
    for (uint32_t i = 0; i < icacheAssoc; i++) {
        if (lines[i].lru < icacheAssoc && lines[i].tag == tag) {
            // cache hit
            for (uint32_t j = 0; j < icacheAssoc; j++) {
                if (lines[j].lru < icacheAssoc && lines[j].lru > lines[i].lru) {
                    lines[j].lru--;
                }
            }
            lines[i].lru = icacheAssoc;
            return;
        }
    }
}

void dcache_invalid(uint32_t addr) {
    if (dcacheSets == 0) {
        return;
    }
    uint32_t tag = addr >> (dBits + blockBits);
    uint32_t loc = (addr >> blockBits) & (dcacheSets - 1u);
    CacheLine *lines = dCache.sets[loc].lines;
    for (uint32_t i = 0; i < dcacheAssoc; i++) {
        if (lines[i].lru < dcacheAssoc && lines[i].tag == tag) {
            // cache hit
            for (uint32_t j = 0; j < dcacheAssoc; j++) {
                if (lines[j].lru < dcacheAssoc && lines[j].lru > lines[i].lru) {
                    lines[j].lru--;
                }
            }
            lines[i].lru = dcacheAssoc;
            return;
        }
    }
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
l2cache_access(uint32_t addr)
{
    if (l2cacheSets == 0) {
        return memspeed;
    }
    l2cacheRefs++;
    uint32_t tag = addr >> (l2Bits + blockBits);
    uint32_t loc = (addr >> blockBits) & (l2cacheSets - 1u);
    CacheLine *lines = l2Cache.sets[loc].lines;

    for (uint32_t i = 0; i < l2cacheAssoc; i++) {
        if (lines[i].lru < l2cacheAssoc && lines[i].tag == tag) {
            // cache hit
            for (uint32_t j = 0; j < l2cacheAssoc; j++) {
                if (lines[j].lru < lines[i].lru) {
                    lines[j].lru++;
                }
            }
            lines[i].lru = 0;
            return l2cacheHitTime;
        }
    }

    // cache miss
    uint32_t penalty = memspeed;

    uint8_t pos = 0; // position to replace on cache miss
    for (uint32_t i = 0; i < l2cacheAssoc; i++) {
        if (lines[i].lru > lines[pos].lru) {
            pos = i;
        }
    }
    if (inclusive && lines[pos].lru < l2cacheAssoc) {
        uint32_t addr = (lines[pos].tag << (l2Bits + blockBits)) | (loc << blockBits);
        icache_invalid(addr);
        dcache_invalid(addr);
    }
    for (uint32_t j = 0; j < l2cacheAssoc; j++) {
        if (lines[j].lru < lines[pos].lru) {
            lines[j].lru++;
        }
    }
    lines[pos].lru = 0;
    lines[pos].tag = tag;

    l2cacheMisses++;
    l2cachePenalties += penalty;
    return l2cacheHitTime + penalty;
}
