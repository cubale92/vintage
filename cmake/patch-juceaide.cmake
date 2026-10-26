# Run as a FetchContent PATCH_COMMAND inside the JUCE source folder (only when ECEE_HEADLESS_CI=ON).
# Hosted Windows CI machines (e.g. GitLab's) run jobs without a desktop session, and juceaide crashes while
# starting JUCE's GUI system there. juceaide never needs a window for plug-ins without icons, so the GUI
# initialiser is removed.
set(main "extras/Build/juceaide/Main.cpp")
file(READ "${main}" src)
string(FIND "${src}" "juce::ScopedJuceInitialiser_GUI libraryInitialiser;" found)
if(found EQUAL -1)
    message(STATUS "8cee: juceaide already patched (or JUCE changed)")
else()
    string(REPLACE "juce::ScopedJuceInitialiser_GUI libraryInitialiser;"
                   "/* 8cee: GUI initialiser removed for headless CI */" src "${src}")
    file(WRITE "${main}" "${src}")
    message(STATUS "8cee: patched juceaide for headless CI")
endif()
