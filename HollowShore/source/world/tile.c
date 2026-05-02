#include "tile.h"

const TileDef tile_defs[TILE_COUNT] = {
    /* TILE_GRASS      */ { TRUE,  RESOURCE_GRASS },
    /* TILE_DIRT       */ { TRUE,  RESOURCE_NONE  },
    /* TILE_WATER      */ { FALSE, RESOURCE_WATER },
    /* TILE_TREE       */ { FALSE, RESOURCE_TREE  },
    /* TILE_ROCK       */ { FALSE, RESOURCE_ROCK  },
    /* TILE_TILLED     */ { TRUE,  RESOURCE_NONE  },
    /* TILE_CROP_1     */ { TRUE,  RESOURCE_NONE  },
    /* TILE_CROP_2     */ { TRUE,  RESOURCE_NONE  },
    /* TILE_CROP_3     */ { TRUE,  RESOURCE_NONE  },
    /* TILE_LAVA       */ { TRUE,  RESOURCE_NONE  },
    /* TILE_ICE        */ { TRUE,  RESOURCE_NONE  },
    /* TILE_CORRUPTION */ { TRUE,  RESOURCE_NONE  },
};
