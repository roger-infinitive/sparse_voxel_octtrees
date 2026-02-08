struct SvoImport {
    int topLevel;
    u32* nodesAtLevel;
    u8** masksAtLevel;
    u32** firstChild;
};

int Popcount8(u8 mask) {
    int popcount = 0;
    for (int j = 0; j < 8; j++) {
        if (mask & (1 << j)) {
            popcount += 1;
        }   
    }
    return popcount;
}

void VerifySvoPopCount(SvoImport* svo) {
    for (int lvl = 0; lvl < svo->topLevel; lvl++) {
        int count = svo->nodesAtLevel[lvl];
        int popcount = 0;
        for (int i = 0; i < count; i++) {
            u8 mask = svo->masksAtLevel[lvl][i];
            popcount += Popcount8(mask);
        }
        ASSERT_ERROR(popcount == svo->nodesAtLevel[lvl + 1], "Incorrect popcount for SVO.\n");
        printf("popcount found/import - %d/%d\n", popcount, svo->nodesAtLevel[lvl + 1]);
    }
}

SvoImport LoadSvo(const char* filePath, AllocFunc alloc) { 
    SvoImport svo = {};
   
    TempArenaMemory arena = TempArenaMemoryBegin(&tempAllocator);
    
    MemoryBuffer mb = {};
    bool loaded = ReadEntireFileAndNullTerminate(filePath, &mb, TempAllocator);
    ASSERT_ERROR(loaded, "Failed to load SVO file: %s\n", filePath);

    char magicNumber[4];
    ReadBytes(&mb, magicNumber, 4);
    ASSERT_ERROR(memcmp(magicNumber, "RSVO", 4) == 0, "Wrong magic number for RSVO.");
    
    s32 version = ReadS32(&mb);
    ASSERT_ERROR(version == 1, "Unsupported version for RSVO.");
    
    s32 reserved0 = ReadS32(&mb);
    s32 reserved1 = ReadS32(&mb);

    svo.topLevel = ReadS32(&mb);

    svo.nodesAtLevel = (u32*)alloc(sizeof(u32) * (svo.topLevel + 1));
    for (int i = 0; i < svo.topLevel + 1; i++) {
        svo.nodesAtLevel[i] = ReadU32(&mb);
    }
    
    ASSERT_ERROR(svo.nodesAtLevel[0] == 1, "Top Level must only have 1 node.");
    
    svo.masksAtLevel = (u8**)alloc(sizeof(u8*) * (svo.topLevel + 1));
    svo.masksAtLevel[0] = 0;

    for (int i = 0; i < svo.topLevel; i++) {
        u32 count = svo.nodesAtLevel[i];
        u8* masks = (u8*)alloc(sizeof(u8) * count);
        ReadBytes(&mb, masks, count);
        svo.masksAtLevel[i] = masks;
    }
    
    ASSERT_ERROR(mb.position == mb.size, "Did not read entire file.");
    
    TempArenaMemoryEnd(arena);
    
    return svo;
}

bool IsFilled(SvoImport* svo, int lvl, Vector3Int c) {
    u32 dim = 1u << lvl;
    if ((u32)c.x >= dim || (u32)c.y >= dim || (u32)c.z >= dim) { 
        return false;
    }
    
    int node = 0;
    for (int i = 0; i < lvl; ++i) {
        int shift = (lvl - 1) - i;
        int xb = (c.x >> shift) & 1;
        int yb = (c.y >> shift) & 1;
        int zb = (c.z >> shift) & 1;
        int child = xb | (yb << 1) | (zb << 2);
        
        u8 mask = svo->masksAtLevel[i][node];
        if ((mask & (1u << child)) == 0) {
            return false; // empty
        }
        
        u8 beforeMask = mask & ((1u << child) - 1u);
        int rank = Popcount8(beforeMask);
        node = svo->firstChild[i][node] + rank;
    }
    
    return true;
}