# This is a "hack" for the fact that SDK.369 includes "vstgui4/vstgui" instead of ${VSTGUI_ROOT}/vstgui4/vstgui
message(STATUS "vstgui4/vstgui -> Hack to include the proper folder.")
add_subdirectory(${VSTGUI_ROOT}/vstgui4/vstgui vstgui4/vstgui)
