cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "")

set(vst3sdk_SOURCE_DIR "/Users/yoon/VST_SDK/vst3sdk")
if(NOT vst3sdk_SOURCE_DIR)
    message(FATAL_ERROR "Path to VST3 SDK is empty!")
endif()

project(VLC_Compressor
    # This is your plug-in version number. Change it here only.
    # Version number symbols usable in C++ can be found in
    # source/version.h and ${PROJECT_BINARY_DIR}/projectversion.h.
    VERSION 1.0.0.0 
    DESCRIPTION "VLC_Compressor VST 3 Plug-in"
)

set(SMTG_VSTGUI_ROOT "${vst3sdk_SOURCE_DIR}")

add_subdirectory(${vst3sdk_SOURCE_DIR} ${PROJECT_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()

smtg_add_vst3plugin(VLC_Compressor
    source/version.h
    source/VLCComp_cids.h
    source/VLCComp_processor.h
    source/VLCComp_processor.cpp
    source/VLCComp_controller.h
    source/VLCComp_controller.cpp
    source/VLCComp_entry.cpp
)

#- VSTGUI Wanted ----
if(SMTG_ENABLE_VSTGUI_SUPPORT)
    target_sources(VLC_Compressor
        PRIVATE
            resource/VLCComp_editor.uidesc
    )
    target_link_libraries(VLC_Compressor
        PRIVATE
            vstgui_support
    )
    smtg_target_add_plugin_resources(VLC_Compressor
        RESOURCES
            "resource/VLCComp_editor.uidesc"
    )
endif(SMTG_ENABLE_VSTGUI_SUPPORT)
# -------------------

smtg_target_add_plugin_snapshots (VLC_Compressor
    RESOURCES
        resource/3101A44EE92E5D4E932A6FC6C08EE6F5_snapshot.png
        resource/3101A44EE92E5D4E932A6FC6C08EE6F5_snapshot_2.0x.png
)

target_link_libraries(VLC_Compressor
    PRIVATE
        sdk
)

smtg_target_configure_version_file(VLC_Compressor)

if(SMTG_MAC)
    smtg_target_set_bundle(VLC_Compressor
        BUNDLE_IDENTIFIER io.github.yg331.VLC.Compressor
        COMPANY_NAME "yg331"
    )
    smtg_target_set_debug_executable(VLC_Compressor
        "/Applications/VST3PluginTestHost.app"
        "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )
elseif(SMTG_WIN)
    target_sources(VLC_Compressor PRIVATE 
        resource/win32resource.rc
    )
    if(MSVC)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT VLC_Compressor)

        smtg_target_set_debug_executable(VLC_Compressor
            "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
            "--pluginfolder \"$(OutDir)/\""
        )
    endif()
endif(SMTG_MAC)

# Add an AUv2 target
if (SMTG_MAC AND XCODE AND SMTG_COREAUDIO_SDK_PATH)
	smtg_target_add_auv2(VLC_Compressor-au
		BUNDLE_NAME VLC_Compressor
		BUNDLE_IDENTIFIER io.github.yg331.VLC.Compressor.audiounit
		INFO_PLIST_TEMPLATE resource/au-info.plist
		VST3_PLUGIN_TARGET VLC_Compressor)
endif(SMTG_MAC AND XCODE AND SMTG_COREAUDIO_SDK_PATH)
