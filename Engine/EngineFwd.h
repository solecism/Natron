/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef Engine_EngineFwd_h
#define Engine_EngineFwd_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <vector>


// boost

namespace boost {
template<class T> class weak_ptr;
template<class T> class shared_ptr;
namespace archive {
class xml_iarchive;
class xml_oarchive;
}
namespace serialization {
class access;
}
}


// Qt

class QByteArray;
class QChar;
class QDateTime;
class QFileInfo;
class QLocalServer;
class QLocalSocket;
class QMutex;
class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;
class QProcess;
class QSettings;
class QString;
class QStringList;
class QThread;
class QTimer;
class QUrl;
class QWaitCondition;

// cairo
typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo_pattern cairo_pattern_t;


// OpenFX

typedef struct OfxImageMemoryStruct *OfxImageMemoryHandle;
typedef struct OfxImageEffectStruct *OfxImageEffectHandle;
typedef struct OfxImageClipStruct *OfxImageClipHandle;

namespace OFX {
namespace Host {
class Plugin;
namespace Property {
class Set;
}
namespace ImageEffect {
class ImageEffectPlugin;
class Descriptor;
class ClipDescriptor;
class OverlayInteract;
}
namespace Interact {
class Descriptor;
class Instance;
}
}
}

// SequenceParsing
namespace SequenceParsing {
class SequenceFromFiles;
}

// Natron Engine

NATRON_NAMESPACE_ENTER
class AbortableRenderInfo;
class AbortableThread;
class AbstractOfxEffectInstance;
class AfterQuitProcessingI;
class AppInstance;
class AppTLS;
class Bezier;
class BezierCP;
class BlockingBackgroundRender;
class BufferableObject;
class CLArgs;
class CacheEntryHolder;
class CacheSignalEmitter;
class CreateNodeArgs;
class ChoiceExtraData;
class Curve;
class Dimension;
class DockablePanelI;
class EffectInstance;
class ExistenceCheckerThread;
class FileSystemItem;
class FileSystemModel;
class Format;
class FrameEntry;
class FramebufferConfig;
class FrameKey;
class FrameParams;
class GPUContextPool;
class GLShader;
class GenericAccess;
class GenericSchedulerThread;
class GenericSchedulerThreadWatcher;
class GenericWatcherCallerArgs;
class Hash64;
class HostOverlayKnobs;
class HostOverlayKnobsCornerPin;
class HostOverlayKnobsPosition;
class HostOverlayKnobsTransform;
class Image;
class ImagePlaneDesc;
class ImageKey;
class ImageParams;
class KeyFrame;
class KnobBool;
class KnobButton;
class KnobChoice;
class KnobColor;
class KnobDouble;
class KnobFactory;
class KnobFile;
class KnobGroup;
class KnobGuiI;
class KnobHelper;
class KnobHolder;
class KnobI;
class KnobInt;
class KnobLayers;
class KnobOutputFile;
class KnobPage;
class KnobParametric;
class KnobPath;
class KnobSeparator;
class KnobSerialization;
class KnobString;
class KnobTable;
class LibraryBinary;
struct LogEntry;
class Node;
class NodeCollection;
class NodeGroup;
class NodeGraphI;
class NodeGuiI;
class NodeMetadata;
class NodeSerialization;
class NodeSettingsPanel;
class OSGLContext;
class OSGLContextAttacher;
struct GLRendererID;
struct OpenGLRendererInfo;
class OfxClipInstance;
class OfxEffectInstance;
class OfxHost;
class OfxImage;
class OfxImageEffectInstance;
class OfxOverlayInteract;
class OfxParamOverlayInteract;
class OfxParamToKnob;
class OfxStringInstance;
class OpenGLViewerI;
class OutputEffectInstance;
class OutputSchedulerThreadStartArgs;
class OverlaySupport;
class ParallelRenderArgsSetter;
class Plugin;
class PluginGroupNode;
class PluginMemory;
class PrecompNode;
class ProcessHandler;
class ProcessInputChannel;
class Project;
struct ProjectBeingLoadedInfo;
class ProjectSerialization;
class RectD;
class RectI;
class RenderEngine;
class RenderStats;
class RenderingFlagSetter;
class RotoContext;
class RotoPaint;
class RotoDrawableItem;
class RotoItem;
class RotoItemSerialization;
class RotoLayer;
class RotoPaintInteract;
class RotoPoint;
class RotoStrokeItem;
class Settings;
class StringAnimationManager;
class TLSHolderBase;
class Texture;
class TextureRect;
struct TileCacheFile;
class TimeLine;
class TrackArgs;
class TrackerFrameAccessor;
struct TrackMarkerAndOptions;
class TrackMarker;
class TrackerContext;
class TrackSerialization;
class TrackerContextSerialization;
class UndoCommand;
class ViewerInstance;
class ViewerCurrentFrameRequestSchedulerStartArgs;
class ViewIdx;
namespace Color {
class Lut;
}
namespace Transform {
struct Matrix3x3;
}

#ifdef __NATRON_WIN32__
struct OSGLContext_wgl_data;
#endif
#ifdef __NATRON_LINUX__
class OSGLContext_glx_data;
#endif

NATRON_PYTHON_NAMESPACE_ENTER
class App;
class AppSettings;
class BezierCurve;
class BooleanParam;
class ButtonParam;
class ChoiceParam;
class ColorParam;
class Double2DParam;
class Double3DParam;
class DoubleParam;
class Effect;
class FileParam;
class Group;
class GroupParam;
class ItemBase;
class ImageLayer;
class Int2DParam;
class Int3DParam;
class IntParam;
class Layer;
class OutputFileParam;
class PathParam;
class Param;
class PageParam;
class ParametricParam;
class Roto;
class SeparatorParam;
class StringParam;
class Track;
class Tracker;
class UserParamHolder;

NATRON_PYTHON_NAMESPACE_EXIT

typedef boost::shared_ptr<AbortableRenderInfo> AbortableRenderInfoPtr;
typedef boost::shared_ptr<AppInstance> AppInstancePtr;
typedef boost::shared_ptr<EffectInstance> EffectInstancePtr;
typedef boost::shared_ptr<FrameEntry> FrameEntryPtr;
typedef boost::shared_ptr<GenericWatcherCallerArgs> WatcherCallerArgsPtr;
typedef boost::shared_ptr<Image> ImagePtr;
typedef boost::shared_ptr<KnobGroup> KnobGroupPtr;
typedef boost::shared_ptr<KnobI> KnobIPtr;
typedef boost::shared_ptr<Node> NodePtr;
typedef boost::shared_ptr<NodeCollection> NodeCollectionPtr;
typedef boost::shared_ptr<NodeGuiI> NodeGuiIPtr;
typedef boost::shared_ptr<OSGLContext> OSGLContextPtr;
typedef boost::shared_ptr<PluginMemory> PluginMemoryPtr;
typedef boost::shared_ptr<Settings> SettingsPtr;
typedef boost::shared_ptr<Texture> GLTexturePtr;
typedef boost::shared_ptr<TrackMarker> TrackMarkerPtr;
typedef boost::shared_ptr<UndoCommand> UndoCommandPtr;
typedef boost::weak_ptr<AbortableRenderInfo> AbortableRenderInfoWPtr;
typedef boost::weak_ptr<AppInstance> AppInstanceWPtr;
typedef boost::weak_ptr<Bezier> BezierWPtr;
typedef boost::weak_ptr<Curve> CurveWPtr;
typedef boost::weak_ptr<EffectInstance> EffectInstanceWPtr;
typedef boost::weak_ptr<FileSystemItem> FileSystemItemWPtr;
typedef boost::weak_ptr<FileSystemModel> FileSystemModelWPtr;
typedef boost::weak_ptr<Image> ImageWPtr;
typedef boost::weak_ptr<KnobBool> KnobBoolWPtr;
typedef boost::weak_ptr<KnobButton> KnobButtonWPtr;
typedef boost::weak_ptr<KnobChoice> KnobChoiceWPtr;
typedef boost::weak_ptr<KnobColor> KnobColorWPtr;
typedef boost::weak_ptr<KnobDouble> KnobDoubleWPtr;
typedef boost::weak_ptr<KnobFile> KnobFileWPtr;
typedef boost::weak_ptr<KnobGroup> KnobGroupWPtr;
typedef boost::weak_ptr<KnobGuiI> KnobGuiIWPtr;
typedef boost::weak_ptr<KnobHolder> KnobHolderWPtr;
typedef boost::weak_ptr<KnobI const> KnobIConstWPtr;
typedef boost::weak_ptr<KnobI> KnobIWPtr;
typedef boost::weak_ptr<KnobInt> KnobIntWPtr;
typedef boost::weak_ptr<KnobLayers> KnobLayersWPtr;
typedef boost::weak_ptr<KnobOutputFile> KnobOutputFileWPtr;
typedef boost::weak_ptr<KnobPage> KnobPageWPtr;
typedef boost::weak_ptr<KnobParametric> KnobParametricWPtr;
typedef boost::weak_ptr<KnobPath> KnobPathWPtr;
typedef boost::weak_ptr<KnobSeparator> KnobSeparatorWPtr;
typedef boost::weak_ptr<KnobString> KnobStringWPtr;
typedef boost::weak_ptr<KnobTable> KnobTableWPtr;
typedef boost::weak_ptr<Node> NodeWPtr;
typedef boost::weak_ptr<NodeCollection> NodeCollectionWPtr;
typedef boost::weak_ptr<NodeGuiI> NodeGuiIWPtr;
typedef boost::weak_ptr<OSGLContext> OSGLContextWPtr;
typedef boost::weak_ptr<OSGLContextAttacher> OSGLContextAttacherWPtr;
typedef boost::weak_ptr<OfxEffectInstance> OfxEffectInstanceWPtr;
typedef boost::weak_ptr<OfxParamOverlayInteract> OfxParamOverlayInteractWPtr;
typedef boost::weak_ptr<OutputEffectInstance> OutputEffectInstanceWPtr;
typedef boost::weak_ptr<OutputSchedulerThreadStartArgs> OutputSchedulerThreadStartArgsWPtr;
typedef boost::weak_ptr<Plugin> PluginWPtr;
typedef boost::weak_ptr<PluginGroupNode> PluginGroupNodeWPtr;
typedef boost::weak_ptr<PluginMemory> PluginMemoryWPtr;
typedef boost::weak_ptr<PrecompNode> PrecompNodeWPtr;
typedef boost::weak_ptr<Project> ProjectWPtr;
typedef boost::weak_ptr<RenderEngine> RenderEngineWPtr;
typedef boost::weak_ptr<RotoContext> RotoContextWPtr;
typedef boost::weak_ptr<RotoDrawableItem> RotoDrawableItemWPtr;
typedef boost::weak_ptr<RotoLayer> RotoLayerWPtr;
typedef boost::weak_ptr<RotoPaint> RotoPaintWPtr;
typedef boost::weak_ptr<RotoPaintInteract> RotoPaintInteractWPtr;
typedef boost::weak_ptr<RotoStrokeItem> RotoStrokeItemWPtr;
typedef boost::weak_ptr<TileCacheFile> TileCacheFileWPtr;
typedef boost::weak_ptr<TimeLine> TimeLineWPtr;
typedef boost::weak_ptr<const TLSHolderBase> TLSHolderBaseConstWPtr;
typedef boost::weak_ptr<TrackMarker> TrackMarkerWPtr;
typedef boost::weak_ptr<TrackerContext> TrackerContextWPtr;
typedef boost::weak_ptr<ViewerInstance> ViewerInstanceWPtr;
typedef std::list<ImagePtr> ImageList;
typedef std::list<NodePtr> NodesList;
typedef std::list<NodeWPtr> NodesWList;
typedef std::vector<KnobIPtr> KnobsVec;

NATRON_NAMESPACE_EXIT

#endif // Engine_EngineFwd_h
