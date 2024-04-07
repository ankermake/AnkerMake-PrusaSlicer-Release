cmake_minimum_required(VERSION 3.13)
project(libslic3r_gui)

include(PrecompiledHeader)

set(SLIC3R_GUI_SOURCES
    pchheader.cpp
    pchheader.hpp
    GUI/AboutDialog.cpp
    GUI/AboutDialog.hpp
    GUI/SysInfoDialog.cpp
    GUI/SysInfoDialog.hpp
    GUI/KBShortcutsDialog.cpp
    GUI/KBShortcutsDialog.hpp
    GUI/BackgroundSlicingProcess.cpp
    GUI/BackgroundSlicingProcess.hpp
    GUI/BitmapCache.cpp
    GUI/BitmapCache.hpp
    GUI/ConfigSnapshotDialog.cpp
    GUI/ConfigSnapshotDialog.hpp
    GUI/3DScene.cpp
    GUI/3DScene.hpp
    GUI/format.hpp
    GUI/GLShadersManager.hpp
    GUI/GLShadersManager.cpp
    GUI/GLShader.cpp
    GUI/GLShader.hpp    
    GUI/GLCanvas3D.hpp
    GUI/GLCanvas3D.cpp
    GUI/SceneRaycaster.hpp
    GUI/SceneRaycaster.cpp	
    GUI/OpenGLManager.hpp
    GUI/OpenGLManager.cpp
    GUI/Selection.hpp
    GUI/Selection.cpp    
    GUI/Gizmos/GLGizmosManager.cpp
    GUI/Gizmos/GLGizmosManager.hpp
    GUI/Gizmos/GLGizmosCommon.cpp
    GUI/Gizmos/GLGizmosCommon.hpp
    GUI/Gizmos/GLGizmoBase.cpp
    GUI/Gizmos/GLGizmoBase.hpp
    GUI/Gizmos/GLGizmoSlaBase.cpp
    GUI/Gizmos/GLGizmoSlaBase.hpp	
    GUI/Gizmos/GLGizmoEmboss.cpp
    GUI/Gizmos/GLGizmoEmboss.hpp
    GUI/Gizmos/GLGizmoMove.cpp
    GUI/Gizmos/GLGizmoMove.hpp
    GUI/Gizmos/GLGizmoRotate.cpp
    GUI/Gizmos/GLGizmoRotate.hpp
    GUI/Gizmos/GLGizmoScale.cpp
    GUI/Gizmos/GLGizmoScale.hpp
    GUI/Gizmos/GLGizmoSlaSupports.cpp
    GUI/Gizmos/GLGizmoSlaSupports.hpp
    GUI/Gizmos/GLGizmoFdmSupports.cpp
    GUI/Gizmos/GLGizmoFdmSupports.hpp
    GUI/Gizmos/GLGizmoFlatten.cpp
    GUI/Gizmos/GLGizmoFlatten.hpp
    GUI/Gizmos/GLGizmoCut.cpp
    GUI/Gizmos/GLGizmoCut.hpp
    GUI/Gizmos/GLGizmoHollow.cpp
    GUI/Gizmos/GLGizmoHollow.hpp
    GUI/Gizmos/GLGizmoPainterBase.cpp
    GUI/Gizmos/GLGizmoPainterBase.hpp
    GUI/Gizmos/GLGizmoSeam.cpp
    GUI/Gizmos/GLGizmoSeam.hpp
    GUI/Gizmos/GLGizmoSimplify.cpp
    GUI/Gizmos/GLGizmoSimplify.hpp
    GUI/Gizmos/GLGizmoMmuSegmentation.cpp
    GUI/Gizmos/GLGizmoMmuSegmentation.hpp
    GUI/Gizmos/GLGizmoMeasure.cpp
    GUI/Gizmos/GLGizmoMeasure.hpp
    GUI/GLSelectionRectangle.cpp
    GUI/GLSelectionRectangle.hpp
    GUI/GLModel.hpp
    GUI/GLModel.cpp
    GUI/GLTexture.hpp
    GUI/GLTexture.cpp
    GUI/GLToolbar.hpp
    GUI/GLToolbar.cpp
    GUI/GCodeViewer.hpp
    GUI/GCodeViewer.cpp    
    GUI/Preferences.cpp
    GUI/Preferences.hpp
    GUI/PresetHints.cpp
    GUI/PresetHints.hpp
    GUI/GUI.cpp
    GUI/GUI.hpp
    GUI/GUI_Init.cpp
    GUI/GUI_Init.hpp
    GUI/GUI_Preview.cpp
    GUI/GUI_Preview.hpp
    GUI/GUI_App.cpp
    GUI/GUI_App.hpp
    GUI/GUI_Utils.cpp
    GUI/GUI_Utils.hpp
    GUI/GUI_Geometry.cpp
    GUI/GUI_Geometry.hpp
    GUI/I18N.cpp
    GUI/I18N.hpp
    GUI/IconManager.cpp
    GUI/IconManager.hpp
    GUI/MainFrame.cpp
    GUI/MainFrame.hpp
    GUI/Plater.cpp
    GUI/Plater.hpp
    GUI/PresetComboBoxes.hpp
    GUI/PresetComboBoxes.cpp
    GUI/BitmapComboBox.hpp
    GUI/BitmapComboBox.cpp
    GUI/SavePresetDialog.hpp
    GUI/SavePresetDialog.cpp
    GUI/PhysicalPrinterDialog.hpp
    GUI/PhysicalPrinterDialog.cpp
    GUI/GUI_Factories.cpp
    GUI/GUI_Factories.hpp
    GUI/GUI_ObjectList.cpp
    GUI/GUI_ObjectList.hpp
    GUI/GalleryDialog.cpp
    GUI/GalleryDialog.hpp
    GUI/GUI_ObjectManipulation.cpp
    GUI/GUI_ObjectManipulation.hpp
    GUI/GUI_ObjectSettings.cpp
    GUI/GUI_ObjectSettings.hpp
    GUI/GUI_ObjectLayers.cpp
    GUI/GUI_ObjectLayers.hpp
    GUI/MeshUtils.cpp
    GUI/MeshUtils.hpp
    GUI/Tab.cpp
    GUI/Tab.hpp
    GUI/ConfigManipulation.cpp
    GUI/ConfigManipulation.hpp
    GUI/Field.cpp
    GUI/Field.hpp
    GUI/OptionsGroup.cpp
    GUI/OptionsGroup.hpp
    GUI/AnkerOptionsGroup.cpp
    GUI/AnkerOptionsGroup.hpp
    GUI/OG_CustomCtrl.cpp
    GUI/OG_CustomCtrl.hpp
    GUI/Anker_OG_CustomCtrl.cpp
    GUI/Anker_OG_CustomCtrl.hpp
    GUI/BedShapeDialog.cpp
    GUI/BedShapeDialog.hpp
    GUI/2DBed.cpp
    GUI/2DBed.hpp
    GUI/3DBed.cpp
    GUI/3DBed.hpp
    GUI/CoordAxes.cpp
    GUI/CoordAxes.hpp
    GUI/Camera.cpp
    GUI/Camera.hpp
    GUI/CameraUtils.cpp
    GUI/CameraUtils.hpp
    GUI/wxExtensions.cpp
    GUI/wxExtensions.hpp
    GUI/ExtruderSequenceDialog.cpp
    GUI/ExtruderSequenceDialog.hpp
    GUI/WipeTowerDialog.cpp
    GUI/WipeTowerDialog.hpp
    GUI/RammingChart.cpp
    GUI/RammingChart.hpp
    GUI/RemovableDriveManager.cpp
    GUI/RemovableDriveManager.hpp
    GUI/SendSystemInfoDialog.cpp
    GUI/SendSystemInfoDialog.hpp
    GUI/AnkerSideBarNew.cpp
    GUI/AnkerSideBarNew.hpp
    GUI/SurfaceDrag.cpp
    GUI/SurfaceDrag.hpp
    GUI/BonjourDialog.cpp
    GUI/BonjourDialog.hpp
    GUI/ButtonsDescription.cpp
    GUI/ButtonsDescription.hpp
    GUI/ImGuiWrapper.hpp
    GUI/ImGuiWrapper.cpp
	GUI/Network/chooseDeviceFrame.h
	GUI/Network/chooseDeviceFrame.cpp
    GUI/Network/MsgText.hpp
    GUI/Network/SystemInfo.hpp
    GUI/Network/SystemInfo.cpp
    Config/Snapshot.cpp
    Config/Snapshot.hpp
    Config/Version.cpp
    Config/Version.hpp  
	Config/AnkerGlobalConfig.hpp
    Utils/ASCIIFolding.cpp
    Utils/ASCIIFolding.hpp
    Utils/Serial.cpp
    Utils/Serial.hpp
    GUI/ConfigWizard.cpp
    GUI/ConfigWizard.hpp
    GUI/ConfigWizard_private.hpp
    GUI/MsgDialog.cpp
    GUI/MsgDialog.hpp
    GUI/UpdateDialogs.cpp
    GUI/UpdateDialogs.hpp
    GUI/FirmwareDialog.cpp
    GUI/FirmwareDialog.hpp
    GUI/PrintHostDialogs.cpp
    GUI/PrintHostDialogs.hpp
    GUI/Jobs/Job.hpp
    GUI/Jobs/Worker.hpp
    GUI/Jobs/BoostThreadWorker.hpp
    GUI/Jobs/BoostThreadWorker.cpp
    GUI/Jobs/UIThreadWorker.hpp
    GUI/Jobs/BusyCursorJob.hpp
    GUI/Jobs/PlaterWorker.hpp
    GUI/Jobs/ArrangeJob.hpp
    GUI/Jobs/ArrangeJob.cpp
    GUI/Jobs/CreateFontNameImageJob.cpp
    GUI/Jobs/CreateFontNameImageJob.hpp
    GUI/Jobs/CreateFontStyleImagesJob.cpp
    GUI/Jobs/CreateFontStyleImagesJob.hpp
    GUI/Jobs/EmbossJob.cpp
    GUI/Jobs/EmbossJob.hpp
    GUI/Jobs/RotoptimizeJob.hpp
    GUI/Jobs/RotoptimizeJob.cpp
    GUI/Jobs/FillBedJob.hpp
    GUI/Jobs/FillBedJob.cpp
    GUI/Jobs/SLAImportJob.hpp
    GUI/Jobs/SLAImportJob.cpp
    GUI/Jobs/ProgressIndicator.hpp
    GUI/Jobs/NotificationProgressIndicator.hpp
    GUI/Jobs/NotificationProgressIndicator.cpp
    GUI/Jobs/ThreadSafeQueue.hpp
    GUI/Jobs/SLAImportDialog.hpp
	GUI/Jobs/OrientJob.hpp
	GUI/Jobs/OrientJob.cpp
    GUI/ProgressStatusBar.hpp
    GUI/ProgressStatusBar.cpp
    GUI/Mouse3DController.cpp
    GUI/Mouse3DController.hpp
    GUI/DoubleSlider.cpp
    GUI/DoubleSlider.hpp
    GUI/Notebook.cpp
    GUI/Notebook.hpp
    GUI/ObjectDataViewModel.cpp
    GUI/ObjectDataViewModel.hpp
    GUI/InstanceCheck.cpp
    GUI/InstanceCheck.hpp
    GUI/Search.cpp
    GUI/Search.hpp
    GUI/NotificationManager.cpp
    GUI/NotificationManager.hpp
    GUI/UnsavedChangesDialog.cpp
    GUI/UnsavedChangesDialog.hpp
    GUI/ExtraRenderers.cpp
    GUI/ExtraRenderers.hpp
    GUI/ProjectDirtyStateManager.hpp
    GUI/ProjectDirtyStateManager.cpp
    GUI/DesktopIntegrationDialog.cpp
    GUI/DesktopIntegrationDialog.hpp
    GUI/HintNotification.cpp
    GUI/HintNotification.hpp
    GUI/FileArchiveDialog.cpp
    GUI/FileArchiveDialog.hpp
    GUI/Downloader.cpp
    GUI/Downloader.hpp
    GUI/DownloaderFileGet.cpp
    GUI/DownloaderFileGet.hpp
	GUI/AnkerWebView.cpp
	GUI/AnkerWebView.hpp
    GUI/AnkerConfig.hpp	
    GUI/AnkerConfig.cpp	
	GUI/AnkerDevice.cpp	
    GUI/AnkerDevice.hpp
    GUI/AnkerBtn.cpp
    GUI/AnkerBtn.hpp
    GUI/AnkerHint.cpp
    GUI/AnkerHint.h
    GUI/AnkerGCodeImportDialog.hpp
    GUI/AnkerGCodeImportDialog.cpp
    GUI/AnkerPrintFinishDialog.hpp
    GUI/AnkerPrintFinishDialog.cpp
	GUI/AnkerTaskPanel.hpp
	GUI/AnkerTaskPanel.cpp
	GUI/AnkerLineEdit.hpp
	GUI/AnkerLineEdit.cpp
    GUI/AnkerVideo.hpp
    GUI/AnkerVideo.cpp
    GUI/AnkerGcodePreviewToolBar.hpp
    GUI/AnkerGcodePreviewToolBar.cpp
    GUI/AnkerGcodePreviewSideBar.hpp
    GUI/AnkerGcodePreviewSideBar.cpp
	GUI/AnkerAdjustItemWidget.hpp
    GUI/AnkerAdjustItemWidget.cpp
	GUI/AnkerTextLabel.hpp
    GUI/AnkerTextLabel.cpp
	GUI/AnkerCustomEvent.hpp
    GUI/AnkerCustomEvent.cpp	
	GUI/AnkerHyperlink.hpp
    GUI/AnkerHyperlink.cpp
	GUI/AnkerShowWidget.hpp
    GUI/AnkerShowWidget.cpp
	GUI/AnkerNavWidget.hpp
    GUI/AnkerNavWidget.cpp
	GUI/AnkerCheckBox.cpp
    GUI/AnkerCheckBox.hpp
    GUI/AnkerEasyPanel.cpp
    GUI/AnkerEasyPanel.hpp
	GUI/AnkerObjectBar.hpp
	GUI/AnkerObjectBar.cpp
	GUI/AnkerObjectBarView.hpp
	GUI/AnkerObjectBarView.cpp
	GUI/AnkerObjectLayers.hpp
	GUI/AnkerObjectLayers.cpp
	GUI/AnkerFloatingList.hpp
	GUI/AnkerFloatingList.cpp
	GUI/AnkerObjectManipulator.hpp
	GUI/AnkerObjectManipulator.cpp
	GUI/AnkerFunctionPanel.cpp
	GUI/AnkerFunctionPanel.h
	GUI/Security/AnkerSecurity.cpp
	GUI/Security/AnkerSecurity.hpp
	GUI/Common/AnkerValidator.hpp
	GUI/Common/AnkerValidator.cpp
	GUI/Common/AnkerSwitchButton.hpp
    GUI/Common/AnkerSwitchButton.cpp
	GUI/Common/AnkerCombinButton.cpp
	GUI/Common/AnkerFont.cpp
	GUI/Common/AnkerFont.hpp
	GUI/Common/AnkerCombinButton.h
	GUI/Common/AnkerGUIConfig.hpp
	GUI/Common/AnkerTitledPanel.hpp
	GUI/Common/AnkerTitledPanel.cpp
	GUI/Common/AnkerSplitCtrl.hpp
	GUI/Common/AnkerSplitCtrl.cpp
	GUI/Common/AnkerMsgDialog.hpp
	GUI/Common/AnkerMsgDialog.cpp
	GUI/Common/AnkerLoadingMask.hpp
	GUI/Common/AnkerLoadingMask.cpp
	GUI/Common/AnkerBox.hpp
	GUI/Common/AnkerBox.cpp
	GUI/Common/AnkerButton.hpp
	GUI/Common/AnkerButton.cpp
	GUI/Common/AnkerComboBox.hpp
	GUI/Common/AnkerComboBox.cpp
	GUI/Common/AnkerDialog.hpp
	GUI/Common/AnkerDialog.cpp
	GUI/Common/AnkerLine.hpp
	GUI/Common/AnkerLine.cpp
	GUI/Common/AnkerSpinBox.hpp
	GUI/Common/AnkerSpinBox.cpp
	GUI/Common/AnkerStaticText.hpp
	GUI/Common/AnkerStaticText.cpp
	GUI/Common/AnkerBase.hpp
	GUI/Common/AnkerBase.cpp
	GUI/Common/AnkerCopyrightDialog.hpp
	GUI/Common/AnkerCopyrightDialog.cpp
	GUI/Common/AnkerListCtrl.hpp
	GUI/Common/AnkerListCtrl.cpp
	GUI/Common/AnkerScrolledWindow.hpp
	GUI/Common/AnkerScrolledWindow.cpp
	GUI/Common/AnkerMulticolorSyncDeviceDialog.hpp
	GUI/Common/AnkerMulticolorSyncDeviceDialog.cpp
	GUI/Common/AnkerSimpleCombox.hpp
	GUI/Common/AnkerSimpleCombox.cpp
	GUI/Common/AnkerBitmapCombox.hpp
	GUI/Common/AnkerBitmapCombox.cpp
	GUI/Common/AnkerPopupWidget.hpp
	GUI/Common/AnkerPopupWidget.cpp
    GUI/Common/AnkerOTANotesBox.hpp
	GUI/Common/AnkerOTANotesBox.cpp
    GUI/Common/AnkerMenuBar.hpp
    GUI/Common/AnkerMenuBar.cpp
	GUI/Common/AnkerLineEditUnit.hpp
	GUI/Common/AnkerLineEditUnit.cpp
    GUI/Common/AnkerFrameModal.hpp
    GUI/Common/AnkerFrameModal.cpp
    GUI/Common/AnkerLoadingLabel.hpp
    GUI/Common/AnkerLoadingLabel.cpp
    GUI/Common/AnkerProgressCtrl.hpp
    GUI/Common/AnkerProgressCtrl.cpp
    GUI/Common/AnkerSliderCtrl.hpp
    GUI/Common/AnkerSliderCtrl.cpp
    GUI/Common/AnkerScrollbar.hpp
    GUI/Common/AnkerScrollbar.cpp

    GUI/Common/AnkerMerterialBoxBtn.hpp
    GUI/Common/AnkerMerterialBoxBtn.cpp
    GUI/Common/AnkerRoundPanel.cpp
    GUI/Common/AnkerRoundPanel.hpp
    GUI/Common/AnkerCombinComboBox.cpp
    GUI/Common/AnkerCombinComboBox.hpp
    GUI/Common/AnkerRoundBaseDialog.cpp
    GUI/Common/AnkerRoundBaseDialog.hpp
    GUI/Common/AnkerRoundDialog.cpp
    GUI/Common/AnkerRoundDialog.hpp
    GUI/Common/AnkerBaseCtls.h
	GUI/Common/AnkerBaseCtls.cpp
	GUI/Common/AnkerTextCtls.h
	GUI/Common/AnkerTextCtls.cpp
	GUI/Common/AnkerNozzlesStausPanel.cpp
    GUI/Common/AnkerNozzlesStausPanel.h
    GUI/Common/AnkerNumberEnterDialog.hpp
    GUI/Common/AnkerNumberEnterDialog.cpp
    GUI/Common/AnkerFeedbackDialog.hpp
    GUI/Common/AnkerFeedbackDialog.cpp
	GUI/Common/AnkerMaterialMappingPanel.cpp
	GUI/Common/AnkerMaterialMappingPanel.h
	GUI/ViewModel/AnkerMaterialMappingViewModel.h
    GUI/GcodeVerify/GcodeVerifyHint.hpp
    GUI/GcodeVerify/GcodeVerifyHint.cpp
    GUI/GcodeVerify/PrintCheckHint.hpp
    GUI/GcodeVerify/PrintCheckHint.cpp
	GUI/PrinterConfigPlater/AnkerParameterPanel.hpp
	GUI/PrinterConfigPlater/AnkerParameterPanel.cpp
	GUI/PrinterConfigPlater/AnkerPrintParaItem.hpp
	GUI/PrinterConfigPlater/AnkerPrintParaItem.cpp	
	GUI/PrinterConfigPlater/AnkerParameterData.hpp
	GUI/PrinterConfigPlater/AnkerParameterData.cpp
    GUI/AnkerConfigDialog/AnkerConfigDialog.hpp
	GUI/AnkerConfigDialog/AnkerConfigDialog.cpp
    GUI/WebWeak/WebDownload.hpp
    GUI/WebWeak/WebDownload.cpp
    GUI/WebWeak/WebDownloadController.hpp
    GUI/WebWeak/WebDownloadController.cpp
    GUI/WebWeak/DownLoadMessage.hpp
    GUI/WebWeak/DownLoadMessage.cpp
    GUI/AnkerCfgTab.hpp
    GUI/AnkerCfgTab.cpp
    GUI/OnlinePreset/OnlinePresetInfo.hpp
    GUI/OnlinePreset/OnlinePresetInfoTool.hpp
    GUI/OnlinePreset/OnlinePresetInfoTool.cpp
    GUI/OnlinePreset/OnlinePresetManager.hpp
    GUI/OnlinePreset/OnlinePresetManager.cpp
    GUI/OnlinePreset/Version.hpp
    GUI/OnlinePreset/VersionTool.hpp
    GUI/OnlinePreset/VersionTool.cpp
    GUI/AnkerNetModule/AnkerNetDownloadDialog.h
    GUI/AnkerNetModule/AnkerNetDownloadDialog.cpp
	GUI/AnkerNetModule/AnkerNetModuleManager.h
    GUI/AnkerNetModule/AnkerNetModuleManager.cpp
    GUI/AnkerNetModule/HttpTool.h
    GUI/AnkerNetModule/HttpTool.cpp
    GUI/AnkerNetModule/BuryDefines.h
	Utils/AppUpdater.cpp
	Utils/Singleton.hpp
    Utils/AppUpdater.hpp
    Utils/Http.cpp
    Utils/Http.hpp
    Utils/FixModelByWin10.cpp
    Utils/FixModelByWin10.hpp
    Utils/OctoPrint.cpp
    Utils/OctoPrint.hpp
    Utils/Duet.cpp
    Utils/Duet.hpp
    Utils/EmbossStyleManager.cpp
    Utils/EmbossStyleManager.hpp
    Utils/EmbossStylesSerializable.cpp
    Utils/EmbossStylesSerializable.hpp
    Utils/FlashAir.cpp
    Utils/FlashAir.hpp
    Utils/FontConfigHelp.cpp
    Utils/FontConfigHelp.hpp
    Utils/GcodeInfo.cpp
    Utils/GcodeInfo.hpp
    Utils/AstroBox.cpp
    Utils/AstroBox.hpp
    Utils/Repetier.cpp
    Utils/Repetier.hpp
    Utils/PrintHost.cpp
    Utils/PrintHost.hpp
    Utils/Bonjour.cpp
    Utils/Bonjour.hpp
    Utils/PresetUpdater.cpp
    Utils/PresetUpdater.hpp
    Utils/Process.cpp
    Utils/Process.hpp
    Utils/RaycastManager.cpp
    Utils/RaycastManager.hpp
    Utils/UndoRedo.cpp
    Utils/UndoRedo.hpp
    Utils/HexFile.cpp
    Utils/HexFile.hpp
    Utils/TCPConsole.cpp
    Utils/TCPConsole.hpp
    Utils/MKS.cpp
    Utils/MKS.hpp
    Utils/WinRegistry.cpp
    Utils/WinRegistry.hpp
    Utils/WxFontUtils.cpp
    Utils/WxFontUtils.hpp
	Utils/DataMangerUi.hpp
	Utils/DataMangerUi.cpp
	Utils/StringHelper.hpp
	Utils/StringHelper.cpp
	Utils/DeviceVersionUtil.hpp
	Utils/DeviceVersionUtil.cpp	
    Utils/JsonHelp.hpp
    Utils/JsonHelp.cpp
    GUI/FilamentMaterial.hpp
    GUI/FilamentMaterial.cpp
    GUI/FilamentMaterialHelper.hpp
    GUI/FilamentMaterialHelper.cpp
    GUI/FilamentMaterialManager.hpp
    GUI/FilamentMaterialManager.cpp
    GUI/FilamentMaterialConvertor.hpp
    GUI/FilamentMaterialConvertor.cpp
	Utils/wxFileTool.hpp
	Utils/wxFileTool.cpp
)


find_package(NanoSVG REQUIRED)


if (APPLE)
    list(APPEND SLIC3R_GUI_SOURCES
            Utils/RetinaHelperImpl.mm
            Utils/MacDarkMode.mm
            Utils/MacUtils.mm
            GUI/RemovableDriveManagerMM.mm
            GUI/RemovableDriveManagerMM.h
            GUI/Mouse3DHandlerMac.mm
            GUI/InstanceCheckMac.mm
            GUI/InstanceCheckMac.h
			GUI/FullScreenToggle.mm
        )
    FIND_LIBRARY(DISKARBITRATION_LIBRARY DiskArbitration)
endif ()

add_library(libslic3r_gui STATIC ${SLIC3R_GUI_SOURCES})

foreach(_source IN ITEMS ${SLIC3R_GUI_SOURCES})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _group_path "${_source_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()


target_link_libraries(libslic3r_gui libslic3r avrdude libcereal imgui GLEW::GLEW OpenGL::GL hidapi libcurl ${wxWidgets_LIBRARIES} NanoSVG::nanosvg NanoSVG::nanosvgrast)

if (MSVC)
    target_link_libraries(libslic3r_gui Setupapi.lib)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(libslic3r_gui ${DBUS_LIBRARIES}) 
elseif (APPLE)
    target_link_libraries(libslic3r_gui ${DISKARBITRATION_LIBRARY})
endif()

message("----include path; $ENV{THIRD_PART_ROOT}/usr/local/include/json ----")
include_directories($ENV{THIRD_PART_ROOT}/usr/local/include/json)
include_directories($ENV{THIRD_PART_ROOT}/usr/local/include/openssl)
if(MSVC)    
    target_link_libraries(libslic3r_gui $ENV{THIRD_PART_ROOT}/usr/local/lib/openssl/win/libcrypto.lib)
    target_link_libraries(libslic3r_gui $ENV{THIRD_PART_ROOT}/usr/local/lib/openssl/win/libssl.lib)
	target_link_libraries(libslic3r_gui $ENV{THIRD_PART_ROOT}/usr/local/lib/jansson/win/jansson.lib)
if(NOT OPEN_SOURCE)		
	target_link_libraries(libslic3r_gui $ENV{THIRD_PART_ROOT}/usr/local/lib/sentry/win/sentry.lib)
endif()	
elseif (APPLE)
    link_directories($ENV{THIRD_PART_ROOT}/usr/local/lib/openssl/mac)
    target_link_libraries(libslic3r_gui $ENV{THIRD_PART_ROOT}/usr/local/lib/openssl/mac/libcrypto.a)
    target_link_libraries(libslic3r_gui $ENV{THIRD_PART_ROOT}/usr/local/lib/openssl/mac/libssl.a)
    target_link_libraries(libslic3r_gui $ENV{THIRD_PART_ROOT}/usr/local/lib/jansson/mac/libjansson.a)
endif()

if (SLIC3R_STATIC)
    # FIXME: This was previously exported by wx-config but the wxWidgets
    # cmake build forgets this and the build fails in debug mode (or on raspberry release)
    target_compile_definitions(libslic3r_gui PUBLIC -DwxDEBUG_LEVEL=0)
endif()

if (SLIC3R_PCH AND NOT SLIC3R_SYNTAXONLY)
    add_precompiled_header(libslic3r_gui pchheader.hpp FORCEINCLUDE)
endif ()

# We need to implement some hacks for wxWidgets and touch the underlying GTK
# layer and sub-libraries. This forces us to use the include locations and
# link these libraries.
if (UNIX AND NOT APPLE)
    find_package(GTK${SLIC3R_GTK} REQUIRED)
    target_include_directories(libslic3r_gui PRIVATE ${GTK${SLIC3R_GTK}_INCLUDE_DIRS})
    target_link_libraries(libslic3r_gui ${GTK${SLIC3R_GTK}_LIBRARIES} fontconfig)
endif ()

# Add a definition so that we can tell we are compiling slic3r.
target_compile_definitions(libslic3r_gui PRIVATE SLIC3R_CURRENTLY_COMPILING_GUI_MODULE)
