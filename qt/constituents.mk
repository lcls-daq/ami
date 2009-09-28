# List targets (if any) for this package
tgtnames := qtclient

# List source files for each target
tgtsrcs_qtclient := Status.cc
tgtsrcs_qtclient += Path.cc
tgtsrcs_qtclient += FeatureRegistry.cc FeatureRegistry_moc.cc
tgtsrcs_qtclient += CExpression.cc
tgtsrcs_qtclient += Calculator.cc Calculator_moc.cc
tgtsrcs_qtclient += DescTH1F.cc DescTH1F_moc.cc
tgtsrcs_qtclient += DescProf.cc DescProf_moc.cc
tgtsrcs_qtclient += DescChart.cc DescChart_moc.cc
tgtsrcs_qtclient += ChannelDefinition.cc ChannelDefinition_moc.cc
tgtsrcs_qtclient += AxisArray.cc
tgtsrcs_qtclient += AxisControl.cc AxisControl_moc.cc
tgtsrcs_qtclient += Control.cc Control_moc.cc
tgtsrcs_qtclient += TransformConstant.cc TransformConstant_moc.cc
tgtsrcs_qtclient += Transform.cc Transform_moc.cc
tgtsrcs_qtclient += Condition.cc Condition_moc.cc
tgtsrcs_qtclient += Filter.cc Filter_moc.cc
tgtsrcs_qtclient += ChannelMath.cc ChannelMath_moc.cc
tgtsrcs_qtclient += QtWaveform.cc QtTH1F.cc QtProf.cc QtChart.cc QtImage.cc QtChannelMath.cc
tgtsrcs_qtclient += PlotFactory.cc
tgtsrcs_qtclient += PlotFrame.cc PlotFrame_moc.cc
tgtsrcs_qtclient += WaveformDisplay.cc WaveformDisplay_moc.cc
tgtsrcs_qtclient += CursorDefinition.cc CursorDefinition_moc.cc
tgtsrcs_qtclient += CursorPlot.cc CursorPlot_moc.cc
tgtsrcs_qtclient += Cursors.cc
tgtsrcs_qtclient += CursorsX.cc CursorsX_moc.cc
tgtsrcs_qtclient += EdgePlot.cc EdgePlot_moc.cc
tgtsrcs_qtclient += EdgeCursor.cc EdgeCursor_moc.cc
tgtsrcs_qtclient += EdgeFinder.cc EdgeFinder_moc.cc
tgtsrcs_qtclient += Client.cc Client_moc.cc
tgtsrcs_qtclient += WaveformClient.cc
tgtsrcs_qtclient += ZoomPlot.cc ZoomPlot_moc.cc
tgtsrcs_qtclient += PeakPlot.cc PeakPlot_moc.cc
tgtsrcs_qtclient += PeakFinder.cc PeakFinder_moc.cc
tgtsrcs_qtclient += ProjectionPlot.cc ProjectionPlot_moc.cc
tgtsrcs_qtclient += ImageFrame.cc ImageFrame_moc.cc
tgtsrcs_qtclient += ImageColorControl.cc ImageColorControl_moc.cc
tgtsrcs_qtclient += ImageScale.cc ImageScale_moc.cc
tgtsrcs_qtclient += ImageDisplay.cc ImageDisplay_moc.cc
tgtsrcs_qtclient += RectangleCursors.cc RectangleCursors_moc.cc
tgtsrcs_qtclient += AnnulusCursors.cc AnnulusCursors_moc.cc
tgtsrcs_qtclient += ImageRPhiProjection.cc ImageRPhiProjection_moc.cc
tgtsrcs_qtclient += ImageXYProjection.cc ImageXYProjection_moc.cc
tgtsrcs_qtclient += ImageClient.cc
tgtsrcs_qtclient += EnvPlot.cc EnvPlot_moc.cc
tgtsrcs_qtclient += EnvClient.cc EnvClient_moc.cc
tgtsrcs_qtclient += DetectorSelect.cc DetectorSelect_moc.cc
tgtsrcs_qtclient += qtclient.cc

# List system libraries (if any) needed by exe_a as <dir>/<lib>. 
# Note that <lib> is the name of the library, not of the file: i.e.
# <lib> for 'libc.so' is 'c'. Low level first.
# tgtslib_exe_a := /usr/lib/rt

qt_libs := qt/QtGui qt/QtCore
qt_libs += qwt/qwt

# List project libraries (if any) needed by exe_a as <project>/<lib>.
# Note that <lib> is the name of the library, not of the file: i.e.
# <lib> for 'libc.so' is 'c'. Low level first.
tgtlibs_qtclient := pdsdata/xtcdata
tgtlibs_qtclient += ami/service ami/data ami/client
tgtlibs_qtclient += $(qt_libs)

tgtlibs_qttest := pdsdata/xtcdata
tgtlibs_qttest += ami/service ami/data ami/client
tgtlibs_qttest += $(qt_libs)

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

tgtincs_qtclient := $(qt_incs)
tgtincs_qttest   := $(qt_incs)

# List system include directories (if any) needed by exe_a as <incdir>.
# tgtsinc_exe_a := /usr/include

# List libraries (if any) for this package
# libnames := lib_a lib_b

# List source files for each library
# libsrcs_lib_a := src_4.cc src_5.cc
# libsrcs_lib_b := src_6.cc

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
# libincs_lib_a := prj_x/include/Linux

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include


