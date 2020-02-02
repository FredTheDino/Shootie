// This file contains all possible settings

// NOTE(ed): There are currently no alternatives here...
#define OPENGL_RENDERER
#define OPENGL_TEXTURE_WIDTH 512
#define OPENGL_TEXTURE_HEIGHT 512
#define OPENGL_TEXTURE_DEPTH 256
#define OPENGL_NUM_LAYERS 16
#define OPENGL_NUM_CAMERAS 2
#define OPENGL_AUTO_APPLY_ASPECTRATIO_CHANGE true
#define MAX_LAYER (OPENGL_NUM_LAYERS - 2)
#define SDL

// If the mouse should warp around the screen allowing you to continue scrolling
// values when tweaking.
#define MOUSE_WARP_IF_CLICKED 1

// The possible inputs
namespace Input {
    enum class Name {
        NONE = 0,

        /* --------- Your bindings go bellow this line --------- */

        LEFT_RIGHT,
        LEFT,
        RIGHT,

        JUMP,
        DIVE,
        SHOOT,

        /* ---------------- And above this line ---------------- */

        // Defaults
        QUIT,
        TWEAK_STEP,
        TWEAK_SMOOTH,

        // Debug
        DEBUG_PERF,
        DEBUG_VIEW,
        DEBUG_VALUES,

        // Editor
        EDIT_MOVE_MODE,
        EDIT_SCALE_MODE,

        EDIT_ABORT,
        EDIT_SELECT_ALL,
        EDIT_SELECT_BOX,
        EDIT_DO,

        COUNT, // Don't write anything after this.
    };
}

// The possible performance timers
namespace Perf {
    enum MarkerID {
        MAIN,
        INPUT,
        RENDER,
        TEXT,

        ENTITY_UPDATE,
        ENTITY_DRAW,
        ENTITY_DEFRAG,

        AUDIO,
        AUDIO_SOURCES,
        AUDIO_EFFECTS,

        NUMBER_OF_MARKERS, // Don't write anything after this.
    };
}


// This is where you add a new entity
namespace Logic {
    enum class EntityType {
        BASE,
        ROBOT,
        BULLET,

        NUM_ENTITY_TYPES, // Don't write anything after this.
    };
}

