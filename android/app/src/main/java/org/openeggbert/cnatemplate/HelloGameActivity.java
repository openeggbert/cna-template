package org.openeggbert.cnatemplate;

import org.libsdl.app.SDLActivity;

/**
 * HelloGame Android entry point.
 *
 * This class just extends SDLActivity so that:
 *   - The application appears in the launcher under its own name/package.
 *   - The SDL3 Java glue (SDLActivity) handles all lifecycle, surface, input,
 *     and audio initialisation automatically.
 *
 * Native code lives in libmain.so (built from src/*.cpp via CMake, see the
 * root CMakeLists.txt's ANDROID branch). SDL3 discovers it by looking for a
 * library named "main".
 *
 * Rename/replace this class along with HelloGame itself when you build your
 * own application on top of this template.
 */
public class HelloGameActivity extends SDLActivity {

    @Override
    protected String[] getLibraries() {
        return new String[]{
                "SDL3",
                "main"
        };
    }
}
