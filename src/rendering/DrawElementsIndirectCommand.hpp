#pragma once

struct DrawElementsIndirectCommand {
    GLuint count;          // indices per chunk (same for every chunk, since topology is shared)
    GLuint instanceCount;  // 1 (not instancing, just batched draws)
    GLuint firstIndex;     // 0 (all chunks share the same index range)
    GLint baseVertex;      // this chunk's offset into the big shared VBO
    GLuint baseInstance;   // 0
};