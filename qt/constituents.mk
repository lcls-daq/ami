# List targets (if any) for this package
#tgtnames := online_ami offline_ami blviewer
tgtnames := online_ami offline_ami

# List source files for each target
tgtsrcs_online_ami += qtclient.cc QOnline.cc QOnline_moc.cc

tgtsrcs_offline_ami := offline_ami.cc XtcRun.cc XtcFileClient.cc XtcFileClient_moc.cc

tgtsrcs_blviewer := blvclient.cc blvclient_moc.cc

# List system libraries (if any) needed by exe_a as <dir>/<lib>. 
# Note that <lib> is the name of the library, not of the file: i.e.
# <lib> for 'libc.so' is 'c'. Low level first.
# tgtslib_exe_a := $(USRLIBDIR)/rt

qt_libs := $(qtlibdir)
qt_libs += qwt/qwt

# List project libraries (if any) needed by exe_a as <project>/<lib>.
# Note that <lib> is the name of the library, not of the file: i.e.
# <lib> for 'libc.so' is 'c'. Low level first.
tgtlibs_online_ami := pdsdata/xtcdata pdsdata/psddl_pdsdata
tgtlibs_online_ami += ami/amisvc ami/amidata ami/server ami/client ami/amicalib ami/amiqt ami/amievent
tgtlibs_online_ami += $(qt_libs)
tgtlibs_online_ami += psalg/psalg
tgtlibs_online_ami += gsl/gsl gsl/gslcblas

datalibs := pdsdata/xtcdata pdsdata/psddl_pdsdata

#
# Need all pdsdata libraries to support dynamic linking of plug-in modules
#
tgtlibs_offline_ami := $(datalibs) pdsdata/appdata pdsdata/anadata pdsdata/indexdata pdsdata/compressdata
tgtlibs_offline_ami += ami/amisvc ami/amidata ami/server ami/client ami/amicalib ami/amievent ami/app ami/amiqt
tgtlibs_offline_ami += $(qt_libs)
tgtlibs_offline_ami += psalg/psalg
tgtlibs_offline_ami += gsl/gsl gsl/gslcblas

tgtlibs_blviewer := $(datalibs) pdsapp/configdb pdsapp/configdbg pds/configdata
tgtlibs_blviewer += ami/amisvc ami/amidata ami/server ami/client ami/amicalib ami/amiqt
tgtlibs_blviewer += psalg/psalg
tgtlibs_blviewer += $(qt_libs)
tgtlibs_blviewer += gsl/gsl gsl/gslcblas

# List special include directories (if any) needed by exe_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
qt_incs := $(qtincdir) $(qwtincs) qwt/include

tgtincs_online_ami  := $(qt_incs) pdsdata/include ndarray/include boost/include
tgtincs_offline_ami := $(qt_incs) pdsdata/include ndarray/include boost/include
tgtincs_blviewer    := $(qt_incs) pdsdata/include

# List system include directories (if any) needed by exe_a as <incdir>.
# tgtsinc_exe_a := /usr/include

# List libraries (if any) for this package
libnames := amiqt

# List source files for each library
libsrcs_amiqt := Status.cc Status_moc.cc
libsrcs_amiqt += Path.cc
libsrcs_amiqt += FeatureRegistry.cc FeatureRegistry_moc.cc
libsrcs_amiqt += AmendedRegistry.cc AmendedRegistry_moc.cc
libsrcs_amiqt += SMPRegistry.cc SMPRegistry_moc.cc
libsrcs_amiqt += SMPWarning.cc SMPWarning_moc.cc
libsrcs_amiqt += QHComboBox.cc QHComboBox_moc.cc
libsrcs_amiqt += FeatureBox.cc FeatureBox_moc.cc
libsrcs_amiqt += QAggSelect.cc QAggSelect_moc.cc
libsrcs_amiqt += QtTree.cc QtTree_moc.cc
libsrcs_amiqt += RunTree.cc RunTree_moc.cc
libsrcs_amiqt += FeatureTree.cc FeatureTree_moc.cc
libsrcs_amiqt += L3Features.cc
libsrcs_amiqt += CExpression.cc
libsrcs_amiqt += Calculator.cc Calculator_moc.cc
libsrcs_amiqt += FeatureCalculator.cc
libsrcs_amiqt += DescTH2T.cc DescTH2T_moc.cc
libsrcs_amiqt += DescTH2F.cc DescTH2F_moc.cc
libsrcs_amiqt += DescTH1F.cc
libsrcs_amiqt += DescBinning.cc DescBinning_moc.cc
libsrcs_amiqt += DescProf.cc DescProf_moc.cc
libsrcs_amiqt += DescProf2D.cc DescProf2D_moc.cc
libsrcs_amiqt += DescScan.cc DescScan_moc.cc
libsrcs_amiqt += DescChart.cc DescChart_moc.cc
libsrcs_amiqt += ScalarPlotDesc.cc ScalarPlotDesc_moc.cc
libsrcs_amiqt += VectorArrayDesc.cc
libsrcs_amiqt += XYHistogramPlotDesc.cc
libsrcs_amiqt += XYProjectionPlotDesc.cc
libsrcs_amiqt += RPhiProjectionPlotDesc.cc
libsrcs_amiqt += QtUtils.cc
libsrcs_amiqt += ChannelDefinition.cc ChannelDefinition_moc.cc
libsrcs_amiqt += AxisInfo.cc AxisArray.cc
libsrcs_amiqt += AxisBins.cc AxisPixels.cc AxisPixelsR.cc
libsrcs_amiqt += AxisControl.cc AxisControl_moc.cc
libsrcs_amiqt += Control.cc Control_moc.cc
libsrcs_amiqt += TransformConstant.cc TransformConstant_moc.cc
libsrcs_amiqt += Transform.cc Transform_moc.cc
libsrcs_amiqt += Condition.cc Condition_moc.cc
libsrcs_amiqt += Contour.cc
libsrcs_amiqt += Filter.cc Filter_moc.cc
libsrcs_amiqt += ChannelMath.cc ChannelMath_moc.cc
libsrcs_amiqt += QtBase.cc
libsrcs_amiqt += QtPlotCurve.cc QtPlotCurve_moc.cc
libsrcs_amiqt += QtWaveform.cc QtTH2F.cc QtTH2F_moc.cc QtTH1F.cc QtProf.cc QtProf2D.cc QtProf2D_moc.cc QtChart.cc QtImage.cc QtScan.cc QtChannelMath.cc QtEmpty.cc QtTable.cc QtTable_moc.cc
libsrcs_amiqt += QtTableDisplay.cc QtTableDisplay_moc.cc
libsrcs_amiqt += PlotFactory.cc
libsrcs_amiqt += PlotFrame.cc PlotFrame_moc.cc
libsrcs_amiqt += PlotFrameF.cc
libsrcs_amiqt += PrintAction.cc PrintAction_moc.cc
libsrcs_amiqt += WaveformDisplay.cc WaveformDisplay_moc.cc
libsrcs_amiqt += CursorDefinition.cc CursorDefinition_moc.cc
libsrcs_amiqt += Fit.cc Fit_moc.cc
libsrcs_amiqt += QFitEntry.cc QLineFitEntry.cc
libsrcs_amiqt += QFit.cc QFit_moc.cc
libsrcs_amiqt += QtPlot.cc QtPlot_moc.cc
libsrcs_amiqt += QtPlotStyle.cc
libsrcs_amiqt += QtPlotSelector.cc QtPlotSelector_moc.cc
libsrcs_amiqt += QtOverlay.cc QtOverlay_moc.cc
libsrcs_amiqt += CursorOp.cc
libsrcs_amiqt += CursorPlot.cc CursorPlot_moc.cc
libsrcs_amiqt += CursorPost.cc CursorOverlay.cc
libsrcs_amiqt += CursorsX.cc CursorsX_moc.cc
libsrcs_amiqt += FFT.cc FFT_moc.cc
#libsrcs_amiqt += EdgePlot.cc EdgePlot_moc.cc
#libsrcs_amiqt += EdgeOverlay.cc
libsrcs_amiqt += Cursor.cc Cursor_moc.cc
libsrcs_amiqt += DoubleEdit.cc DoubleEdit_moc.cc
libsrcs_amiqt += EdgeFinder.cc EdgeFinder_moc.cc
libsrcs_amiqt += CurveFitPost.cc CurveFitOverlay.cc
libsrcs_amiqt += CurveFitPlot.cc CurveFitPlot_moc.cc
libsrcs_amiqt += CurveFit.cc CurveFit_moc.cc
libsrcs_amiqt += QtPWidget.cc QtPWidget_moc.cc 
libsrcs_amiqt += QtPStack.cc QtPStack_moc.cc 
libsrcs_amiqt += AbsClient.cc AbsClient_moc.cc
libsrcs_amiqt += Client.cc Client_moc.cc
libsrcs_amiqt += WaveformClient.cc
libsrcs_amiqt += ZoomPlot.cc ZoomPlot_moc.cc
libsrcs_amiqt += PeakFit.cc PeakFit_moc.cc
libsrcs_amiqt += PeakFitPlot.cc PeakFitPlot_moc.cc
libsrcs_amiqt += PeakFitPost.cc PeakFitOverlay.cc
libsrcs_amiqt += PeakPlot.cc PeakPlot_moc.cc
libsrcs_amiqt += PeakFinder.cc PeakFinder_moc.cc
libsrcs_amiqt += BlobFinder.cc BlobFinder_moc.cc
libsrcs_amiqt += ProjectionPlot.cc ProjectionPlot_moc.cc
libsrcs_amiqt += TwoDPlot.cc TwoDPlot_moc.cc
libsrcs_amiqt += ImageGrid.cc
libsrcs_amiqt += ImageFrame.cc ImageFrame_moc.cc
libsrcs_amiqt += OffloadEngine.cc OffloadEngine_moc.cc
libsrcs_amiqt += ImageInspect.cc ImageInspect_moc.cc
libsrcs_amiqt += ImageColorControl.cc ImageColorControl_moc.cc
libsrcs_amiqt += ImageXYControl.cc ImageXYControl_moc.cc
libsrcs_amiqt += ColorMaps.cc
libsrcs_amiqt += ImageScale.cc ImageScale_moc.cc
libsrcs_amiqt += ImageDisplay.cc ImageDisplay_moc.cc
libsrcs_amiqt += Rect.cc RectROI.cc RectROI_moc.cc
libsrcs_amiqt += RectROIDesc.cc RectROIDesc_moc.cc
libsrcs_amiqt += RectangleCursors.cc RectangleCursors_moc.cc
libsrcs_amiqt += AnnulusCursors.cc AnnulusCursors_moc.cc
libsrcs_amiqt += Rotator.cc
libsrcs_amiqt += ImageIntegral.cc ImageContrast.cc
libsrcs_amiqt += ImageFunctions.cc ImageFunctions_moc.cc
libsrcs_amiqt += ImageContourProjection.cc ImageContourProjection_moc.cc
libsrcs_amiqt += ImageRPhiProjection.cc ImageRPhiProjection_moc.cc
libsrcs_amiqt += ImageXYProjection.cc ImageXYProjection_moc.cc
libsrcs_amiqt += CrossHair.cc CrossHair_moc.cc
libsrcs_amiqt += CrossHairDelta.cc CrossHairDelta_moc.cc
libsrcs_amiqt += ImageGridScale.cc ImageGridScale_moc.cc
libsrcs_amiqt += ImageClient.cc
libsrcs_amiqt += CspadClient.cc CspadClient_moc.cc
libsrcs_amiqt += EpixClient.cc EpixClient_moc.cc
libsrcs_amiqt += FccdClient.cc FccdClient_moc.cc
libsrcs_amiqt += JungfrauClient.cc JungfrauClient_moc.cc
libsrcs_amiqt += FrameClient.cc FrameClient_moc.cc
libsrcs_amiqt += PnccdCalibrator.cc PnccdCalibrator_moc.cc
libsrcs_amiqt += PnccdClient.cc PnccdClient_moc.cc
libsrcs_amiqt += EnvOp.cc
libsrcs_amiqt += EnvPlot.cc EnvPlot_moc.cc EnvTable.cc EnvTable_moc.cc
libsrcs_amiqt += EnvPost.cc EnvOverlay.cc
libsrcs_amiqt += PostAnalysis.cc
libsrcs_amiqt += EnvClient.cc EnvClient_moc.cc
libsrcs_amiqt += LineFitPlotDesc.cc LineFitPlotDesc_moc.cc
libsrcs_amiqt += LineFitClient.cc LineFitClient_moc.cc
libsrcs_amiqt += TdcPlot.cc TdcPlot_moc.cc
libsrcs_amiqt += TdcClient.cc TdcClient_moc.cc
#libsrcs_amiqt += ScriptClient.cc ScriptClient_moc.cc
libsrcs_amiqt += SummaryClient.cc SummaryClient_moc.cc
#libsrcs_amiqt += DetectorGroup.cc DetectorGroup_moc.cc
#libsrcs_amiqt += DetectorSave.cc
#libsrcs_amiqt += DetectorReset.cc
libsrcs_amiqt += DetectorListItem.cc
libsrcs_amiqt += Defaults.cc
libsrcs_amiqt += StatCalculator.cc StatCalculator_moc.cc
libsrcs_amiqt += RateCalculator.cc CpuCalculator.cc
libsrcs_amiqt += RecvCalculator.cc RecvCalculator_moc.cc
libsrcs_amiqt += RateDisplay.cc RunMaster.cc
libsrcs_amiqt += FilterSetup.cc FilterSetup_moc.cc
libsrcs_amiqt += PWidgetManager.cc PWidgetManager_moc.cc
libsrcs_amiqt += ControlLog.cc ControlLog_moc.cc
libsrcs_amiqt += DetectorSelect.cc DetectorSelect_moc.cc
libsrcs_amiqt += MaskFrame.cc MaskFrame_moc.cc
libsrcs_amiqt += MaskDisplay.cc MaskDisplay_moc.cc
libsrcs_amiqt += Droplet.cc Droplet_moc.cc
libsrcs_amiqt += VAConfigApp.cc VAConfigApp_moc.cc

# List special include directories (if any) needed by lib_a as
# <project>/<incdir>. Note that the top level release directory is
# already in the search path.
libincs_amiqt := $(qt_incs) qwt/include  ndarray/include boost/include pdsdata/include gsl/include psalg/include
libsinc_amiqt := $(qwtsinc)

# List system include directories (if any) needed by lib_a as <incdir>.
# libsinc_lib_a := /usr/include


