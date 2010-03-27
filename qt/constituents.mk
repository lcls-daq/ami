# List targets (if any) for this package
tgtnames := online_ami offline_ami

# List source files for each target
tgtsrcs_online_ami += qtclient.cc

tgtsrcs_offline_ami := FileSelect.cc FileSelect_moc.cc
tgtsrcs_offline_ami += XtcFileClient.cc XtcFileClient_moc.cc
tgtsrcs_offline_ami += qtami.cc

# List system libraries (if any) needed by exe_a as <dir>/<lib>. 
# Note that <lib> is the name of the library, not of the file: i.e.
# <lib> for 'libc.so' is 'c'. Low level first.
# tgtslib_exe_a := /usr/lib/rt

qt_libs := qt/QtGui qt/QtCore
qt_libs += qwt/qwt

# List project libraries (if any) needed by exe_a as <project>/<lib>.
# Note that <lib> is the name of the library, not of the file: i.e.
# <lib> for 'libc.so' is 'c'. Low level first.
tgtlibs_online_ami := pdsdata/xtcdata
tgtlibs_online_ami += ami/service ami/data ami/client ami/amiqt
tgtlibs_online_ami += $(qt_libs)

tgtlibs_offline_ami := pdsdata/xtcdata pdsdata/acqdata
tgtlibs_offline_ami += pdsdata/camdata pdsdata/opal1kdata
tgtlibs_offline_ami += pdsdata/pnccddata
tgtlibs_offline_ami += pdsdata/controldata pdsdata/epics
tgtlibs_offline_ami += ami/service ami/data ami/server ami/client ami/event ami/app ami/amiqt
tgtlibs_offline_ami += $(qt_libs)

# List special include directories (if any) needed by exe_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
qt_incs := qt/include qwt/include
# qwt includes qt headers without package prefix!
qt_incs += qt/include/Qt
qt_incs += qt/include/Qt3Support
qt_incs += qt/include/QtAssistant
qt_incs += qt/include/QtCore
qt_incs += qt/include/QtDesigner
qt_incs += qt/include/QtGui
qt_incs += qt/include/QtNetwork
qt_incs += qt/include/QtOpenGL
qt_incs += qt/include/QtScript
qt_incs += qt/include/QtSql
qt_incs += qt/include/QtSvg
qt_incs += qt/include/QtTest
qt_incs += qt/include/QtUiTools
qt_incs += qt/include/QtXml

tgtincs_online_ami  := $(qt_incs)
tgtincs_offline_ami := $(qt_incs)

# List system include directories (if any) needed by exe_a as <incdir>.
# tgtsinc_exe_a := /usr/include

# List libraries (if any) for this package
libnames := amiqt

# List source files for each library
libsrcs_amiqt := Status.cc Status_moc.cc
libsrcs_amiqt += Path.cc
libsrcs_amiqt += FeatureRegistry.cc FeatureRegistry_moc.cc
libsrcs_amiqt += FeatureBox.cc FeatureBox_moc.cc
libsrcs_amiqt += CExpression.cc
libsrcs_amiqt += Calculator.cc Calculator_moc.cc
libsrcs_amiqt += DescTH1F.cc DescTH1F_moc.cc
libsrcs_amiqt += DescProf.cc DescProf_moc.cc
libsrcs_amiqt += DescScan.cc DescScan_moc.cc
libsrcs_amiqt += DescChart.cc DescChart_moc.cc
libsrcs_amiqt += ScalarPlotDesc.cc
libsrcs_amiqt += XYProjectionPlotDesc.cc
libsrcs_amiqt += RPhiProjectionPlotDesc.cc
libsrcs_amiqt += QtPersistent.cc
libsrcs_amiqt += ChannelDefinition.cc ChannelDefinition_moc.cc
libsrcs_amiqt += AxisArray.cc
libsrcs_amiqt += AxisBins.cc
libsrcs_amiqt += AxisControl.cc AxisControl_moc.cc
libsrcs_amiqt += Control.cc Control_moc.cc
libsrcs_amiqt += TransformConstant.cc TransformConstant_moc.cc
libsrcs_amiqt += Transform.cc Transform_moc.cc
libsrcs_amiqt += Condition.cc Condition_moc.cc
libsrcs_amiqt += Contour.cc
libsrcs_amiqt += Filter.cc Filter_moc.cc
libsrcs_amiqt += ChannelMath.cc ChannelMath_moc.cc
libsrcs_amiqt += QtWaveform.cc QtTH1F.cc QtProf.cc QtChart.cc QtImage.cc QtScan.cc QtChannelMath.cc
libsrcs_amiqt += PlotFactory.cc
libsrcs_amiqt += PlotFrame.cc PlotFrame_moc.cc
libsrcs_amiqt += PrintAction.cc PrintAction_moc.cc
libsrcs_amiqt += WaveformDisplay.cc WaveformDisplay_moc.cc
libsrcs_amiqt += CursorDefinition.cc CursorDefinition_moc.cc
libsrcs_amiqt += QtPlot.cc QtPlot_moc.cc
libsrcs_amiqt += CursorPlot.cc CursorPlot_moc.cc
libsrcs_amiqt += CursorsX.cc CursorsX_moc.cc
libsrcs_amiqt += EdgePlot.cc EdgePlot_moc.cc
libsrcs_amiqt += EdgeCursor.cc EdgeCursor_moc.cc
libsrcs_amiqt += EdgeFinder.cc EdgeFinder_moc.cc
libsrcs_amiqt += QtPWidget.cc QtPWidget_moc.cc
libsrcs_amiqt += AbsClient.cc AbsClient_moc.cc
libsrcs_amiqt += Client.cc Client_moc.cc
libsrcs_amiqt += WaveformClient.cc
libsrcs_amiqt += ZoomPlot.cc ZoomPlot_moc.cc
libsrcs_amiqt += PeakFit.cc PeakFit_moc.cc
libsrcs_amiqt += PeakFitPlot.cc PeakFitPlot_moc.cc
libsrcs_amiqt += PeakPlot.cc PeakPlot_moc.cc
libsrcs_amiqt += PeakFinder.cc PeakFinder_moc.cc
libsrcs_amiqt += ProjectionPlot.cc ProjectionPlot_moc.cc
libsrcs_amiqt += ImageFrame.cc ImageFrame_moc.cc
libsrcs_amiqt += ImageColorControl.cc ImageColorControl_moc.cc
libsrcs_amiqt += ImageScale.cc ImageScale_moc.cc
libsrcs_amiqt += ImageDisplay.cc ImageDisplay_moc.cc
libsrcs_amiqt += RectangleCursors.cc RectangleCursors_moc.cc
libsrcs_amiqt += AnnulusCursors.cc AnnulusCursors_moc.cc
libsrcs_amiqt += ImageContourProjection.cc ImageContourProjection_moc.cc
libsrcs_amiqt += ImageRPhiProjection.cc ImageRPhiProjection_moc.cc
libsrcs_amiqt += ImageXYProjection.cc ImageXYProjection_moc.cc
libsrcs_amiqt += ImageClient.cc
libsrcs_amiqt += EnvPlot.cc EnvPlot_moc.cc
libsrcs_amiqt += EnvClient.cc EnvClient_moc.cc
libsrcs_amiqt += DetectorGroup.cc DetectorGroup_moc.cc
libsrcs_amiqt += DetectorSave.cc
libsrcs_amiqt += DetectorReset.cc
libsrcs_amiqt += DetectorListItem.cc
libsrcs_amiqt += DetectorSelect.cc DetectorSelect_moc.cc

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
libincs_amiqt := $(qt_incs)

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include


