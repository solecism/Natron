/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "OfxClipInstance.h"

#include <cfloat>
#include <limits>
#include <bitset>
#include <cassert>
#include <stdexcept>

#include <QtCore/QTextStream>
#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>

#include "Engine/CacheEntry.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/TimeLine.h"
#include "Engine/Hash64.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/RotoContext.h"
#include "Engine/Transform.h"
#include "Engine/TLSHolder.h"
#include "Engine/Project.h"
#include "Engine/ViewIdx.h"

#include <nuke/fnOfxExtensions.h>
#include <ofxOpenGLRender.h>
#include <ofxNatron.h>

NATRON_NAMESPACE_ENTER;

struct OfxClipInstancePrivate
{
    OfxClipInstance* _publicInterface; // can not be a smart ptr
    boost::weak_ptr<OfxEffectInstance> nodeInstance;
    OfxImageEffectInstance* const effect;
    double aspectRatio;
    bool optional;
    bool mask;
    boost::shared_ptr<TLSHolder<OfxClipInstance::ClipTLSData> > tlsData;

    OfxClipInstancePrivate(OfxClipInstance* publicInterface,
                           const OfxEffectInstancePtr& nodeInstance,
                           OfxImageEffectInstance* effect)
        : _publicInterface(publicInterface)
        , nodeInstance(nodeInstance)
        , effect(effect)
        , aspectRatio(1.)
        , optional(false)
        , mask(false)
        , tlsData( new TLSHolder<OfxClipInstance::ClipTLSData>() )
    {
    }

    const std::vector<std::string>& getComponentsPresentInternal(const OfxClipInstance::ClipDataTLSPtr& tls) const;
};

OfxClipInstance::OfxClipInstance(const OfxEffectInstancePtr& nodeInstance,
                                 OfxImageEffectInstance* effect,
                                 int /*index*/,
                                 OFX::Host::ImageEffect::ClipDescriptor* desc)
    : OFX::Host::ImageEffect::ClipInstance(effect, *desc)
    , _imp( new OfxClipInstancePrivate(this, nodeInstance, effect) )
{
    assert(nodeInstance && effect);
    _imp->optional = isOptional();
    _imp->mask = isMask();
}

OfxClipInstance::~OfxClipInstance()
{
}

// callback which should update label
void
OfxClipInstance::setLabel()
{
    OfxEffectInstancePtr effect = _imp->nodeInstance.lock();
    if (effect) {
        int inputNb = getInputNb();
        if (inputNb >= 0) {
            effect->onClipLabelChanged(inputNb, getLabel());
        }
    }
}

// callback which should set secret state as appropriate
void OfxClipInstance::setSecret()
{
    OfxEffectInstancePtr effect = _imp->nodeInstance.lock();
    if (effect) {
        int inputNb = getInputNb();
        if (inputNb >= 0) {
            effect->onClipSecretChanged(inputNb, isSecret());
        }
    }
}

// callback which should update hint
void OfxClipInstance::setHint()
{
    OfxEffectInstancePtr effect = _imp->nodeInstance.lock();
    if (effect) {
        int inputNb = getInputNb();
        if (inputNb >= 0) {
            effect->onClipHintChanged(inputNb, getHint());
        }
    }
}

bool
OfxClipInstance::getIsOptional() const
{
    return _imp->optional;
}

bool
OfxClipInstance::getIsMask() const
{
    return _imp->mask;
}

const std::string &
OfxClipInstance::getUnmappedBitDepth() const
{
    static const std::string byteStr(kOfxBitDepthByte);
    static const std::string shortStr(kOfxBitDepthShort);
    static const std::string halfStr(kOfxBitDepthHalf);
    static const std::string floatStr(kOfxBitDepthFloat);
    static const std::string noneStr(kOfxBitDepthNone);
    EffectInstancePtr inputNode = getAssociatedNode();

    if (inputNode) {
        ///Get the input node's output preferred bit depth
        ImageBitDepthEnum depth = inputNode->getBitDepth(-1);

        switch (depth) {
        case eImageBitDepthByte:

            return byteStr;
            break;
        case eImageBitDepthShort:

            return shortStr;
            break;
        case eImageBitDepthHalf:

            return halfStr;
            break;
        case eImageBitDepthFloat:

            return floatStr;
            break;
        default:
            break;
        }
    }

    ///Return the hightest bit depth supported by the plugin
    EffectInstancePtr effect = getEffectHolder();
    if (effect) {
        const std::string& ret = natronsDepthToOfxDepth( effect->getNode()->getClosestSupportedBitDepth(eImageBitDepthFloat) );
        if (ret == floatStr) {
            return floatStr;
        } else if (ret == shortStr) {
            return shortStr;
        } else if (ret == byteStr) {
            return byteStr;
        }
    }

    return noneStr;
} // OfxClipInstance::getUnmappedBitDepth

const std::string &
OfxClipInstance::getUnmappedComponents() const
{
    static const std::string rgbStr(kOfxImageComponentRGB);
    static const std::string noneStr(kOfxImageComponentNone);
    static const std::string rgbaStr(kOfxImageComponentRGBA);
    static const std::string alphaStr(kOfxImageComponentAlpha);
    EffectInstancePtr inputNode = getAssociatedNode();

    if (inputNode) {
        ///Get the input node's output preferred bit depth and componentns
        ClipDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
        ImageComponents comp = inputNode->getComponents(-1);


        //default to RGBA
        if (comp.getNumComponents() == 0) {
            comp = ImageComponents::getRGBAComponents();
        }
        tls->unmappedComponents = natronsComponentsToOfxComponents(comp);

        return tls->unmappedComponents;
    } else {
        ///The node is not connected but optional, return the closest supported components
        ///of the first connected non optional input.
        if (_imp->optional) {
            OfxEffectInstancePtr effect = _imp->nodeInstance.lock();
            assert(effect);
            int nInputs = effect->getMaxInputCount();
            for (int i  = 0; i < nInputs; ++i) {
                OfxClipInstance* clip = effect->getClipCorrespondingToInput(i);
                if ( clip && !clip->getIsOptional() && clip->getConnected() && (clip->getComponents() != noneStr) ) {
                    return clip->getComponents();
                }
            }
        }


        // last-resort: black and transparent image means RGBA.
        return rgbaStr;
    }
}

// PreMultiplication -
//
//  kOfxImageOpaque - the image is opaque and so has no premultiplication state
//  kOfxImagePreMultiplied - the image is premultiplied by it's alpha
//  kOfxImageUnPreMultiplied - the image is unpremultiplied
const std::string &
OfxClipInstance::getPremult() const
{
    EffectInstancePtr effect = getEffectHolder();

    if (!effect) {
        return natronsPremultToOfxPremult(eImagePremultiplicationPremultiplied);
    }
    if ( isOutput() ) {
        return natronsPremultToOfxPremult( effect->getPremult() );
    } else {
        EffectInstancePtr associatedNode = getAssociatedNode();

        return associatedNode ? natronsPremultToOfxPremult( associatedNode->getPremult() ) : natronsPremultToOfxPremult(eImagePremultiplicationPremultiplied);
    }
}

const std::vector<std::string>&
OfxClipInstancePrivate::getComponentsPresentInternal(const OfxClipInstance::ClipDataTLSPtr& tls) const
{
    tls->componentsPresent.clear();

    EffectInstance::ComponentsAvailableMap compsAvailable;
    EffectInstancePtr effect = _publicInterface->getAssociatedNode();
    if (!effect) {
        return tls->componentsPresent;
    }
    double time = effect->getCurrentTime();

    effect->getComponentsAvailable(true, !_publicInterface->isOutput(), time, &compsAvailable);
    //   } // if (isOutput())

    for (EffectInstance::ComponentsAvailableMap::iterator it = compsAvailable.begin(); it != compsAvailable.end(); ++it) {
        tls->componentsPresent.push_back( OfxClipInstance::natronsComponentsToOfxComponents(it->first) );
    }

    return tls->componentsPresent;
}

// overridden from OFX::Host::ImageEffect::ClipInstance
/*
 * We have to use TLS here because the OpenFX API necessitate that strings
 * live through the entire duration of the calling action. The is the only way
 * to have it thread-safe and local to a current calling time.
 */
const std::vector<std::string>&
OfxClipInstance::getComponentsPresent() const OFX_EXCEPTION_SPEC
{
    try {
        //The components present have just been computed in the previous call to getDimension()
        //so we are fine here
        ClipDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();

        return tls->componentsPresent;
    } catch (...) {
        throw OFX::Host::Property::Exception(kOfxStatErrUnknown);
    }
}

// overridden from OFX::Host::ImageEffect::ClipInstance
int
OfxClipInstance::getDimension(const std::string &name) const OFX_EXCEPTION_SPEC
{
    if (name != kFnOfxImageEffectPropComponentsPresent) {
        return OFX::Host::ImageEffect::ClipInstance::getDimension(name);
    }
    try {
        ClipDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
        const std::vector<std::string>& components = _imp->getComponentsPresentInternal(tls);

        return (int)components.size();
    } catch (...) {
        throw OFX::Host::Property::Exception(kOfxStatErrUnknown);
    }
}

const std::string &
OfxClipInstance::getComponents() const
{
    /*
       The property returned by the clip might differ from the one held on the image if the associated effect
       is identity or if the effect is multi-planar
     */
    return _components;
}

// overridden from OFX::Host::ImageEffect::ClipInstance
// Pixel Aspect Ratio -
//
//  The pixel aspect ratio of a clip or image.
double
OfxClipInstance::getAspectRatio() const
{
    /*
       The property returned by the clip might differ from the one held on the image if the associated effect
       is identity
     */
    return _imp->aspectRatio;
}

void
OfxClipInstance::setAspectRatio(double par)
{
    //This is protected by the clip preferences read/write lock in OfxEffectInstance
    _imp->aspectRatio = par;
}

// Frame Rate -
double
OfxClipInstance::getFrameRate() const
{
    /*
       The frame rate property cannot be held onto images, hence return the "actual" frame rate,
       taking into account the node from which the image came from wrt the identity state
     */
    EffectInstancePtr effect = getEffectHolder();

    if ( isOutput() ) {
        return effect->getFrameRate();
    }

    EffectInstancePtr inputNode = getAssociatedNode();
    if (inputNode) {
        inputNode = inputNode->getNearestNonIdentity( effect->getCurrentTime() );
    }
    if (!inputNode) {
        return effect->getApp()->getProjectFrameRate();
    } else {
        return inputNode->getFrameRate();
    }
}

// overridden from OFX::Host::ImageEffect::ClipInstance
// Frame Range (startFrame, endFrame) -
//
//  The frame range over which a clip has images.
void
OfxClipInstance::getFrameRange(double &startFrame,
                               double &endFrame) const
{
    EffectInstancePtr n = getAssociatedNode();

    if (n) {
        double time;
        ViewIdx view;
        n->getCurrentTimeView(&time, &view);
        U64 hash;
        bool gotHash = n->getRenderHash(time, view, &hash);
        (void)gotHash;
        n->getFrameRange_public(hash, &startFrame, &endFrame);
    } else {
        n = getEffectHolder();
        double first, last;
        n->getApp()->getFrameRange(&first, &last);
        startFrame = first;
        endFrame = last;
    }
}

// overridden from OFX::Host::ImageEffect::ClipInstance
/// Field Order - Which spatial field occurs temporally first in a frame.
/// \returns
///  - kOfxImageFieldNone - the clip material is unfielded
///  - kOfxImageFieldLower - the clip material is fielded, with image rows 0,2,4.... occuring first in a frame
///  - kOfxImageFieldUpper - the clip material is fielded, with image rows line 1,3,5.... occuring first in a frame
const std::string &
OfxClipInstance::getFieldOrder() const
{
    EffectInstancePtr effect = getEffectHolder();

    if (!effect) {
        return natronsFieldingToOfxFielding(eImageFieldingOrderNone);
    }
    if ( isOutput() ) {
        return natronsFieldingToOfxFielding( effect->getFieldingOrder() );
    } else {
        EffectInstancePtr associatedNode = getAssociatedNode();

        return associatedNode ? natronsFieldingToOfxFielding( associatedNode->getFieldingOrder() ) : natronsFieldingToOfxFielding(eImageFieldingOrderNone);
    }
}

// overridden from OFX::Host::ImageEffect::ClipInstance
// Connected -
//
//  Says whether the clip is actually connected at the moment.
bool
OfxClipInstance::getConnected() const
{
    ///a roto brush is always connected
    EffectInstancePtr effect = getEffectHolder();

    assert(effect);



    if (_isOutput) {
        return effect->hasOutputConnected();
    } else {
        int inputNb = getInputNb();
        EffectInstancePtr input;

        if ( !effect->getNode()->isMaskEnabled(inputNb) ) {
            return false;
        }
        ImageComponents comps;
        NodePtr maskInput;
        effect->getNode()->getMaskChannel(inputNb, &comps, &maskInput);
        if (maskInput) {
            input = maskInput->getEffectInstance();
        }

        if (!input) {
            input = effect->getInput(inputNb);
        }

        return input != NULL;
    }

}

// overridden from OFX::Host::ImageEffect::ClipInstance
// Unmapped Frame Rate -
//
//  The unmaped frame range over which an output clip has images.
double
OfxClipInstance::getUnmappedFrameRate() const
{
    EffectInstancePtr inputNode = getAssociatedNode();

    if (inputNode) {
        ///Get the input node  preferred frame rate
        return inputNode->getFrameRate();
    } else {
        ///The node is not connected, return project frame rate
        return getEffectHolder()->getApp()->getProjectFrameRate();
    }
}

// overridden from OFX::Host::ImageEffect::ClipInstance
// Unmapped Frame Range -
//
//  The unmaped frame range over which an output clip has images.
// this is applicable only to hosts and plugins that allow a plugin to change frame rates
void
OfxClipInstance::getUnmappedFrameRange(double &unmappedStartFrame,
                                       double &unmappedEndFrame) const
{
    EffectInstancePtr inputNode = getAssociatedNode();

    if (inputNode) {
        ///Get the input node  preferred frame range
        double time;
        ViewIdx view;
        inputNode->getCurrentTimeView(&time, &view);
        U64 hash;
        bool gotHash = inputNode->getRenderHash(time, view, &hash);
        (void)gotHash;
        return inputNode->getFrameRange_public(hash, &unmappedStartFrame, &unmappedEndFrame);
    } else {
        ///The node is not connected, return project frame range
        return getEffectHolder()->getApp()->getProject()->getFrameRange(&unmappedStartFrame, &unmappedEndFrame);
    }
}

// Continuous Samples -
//
//  0 if the images can only be sampled at discreet times (eg: the clip is a sequence of frames),
//  1 if the images can only be sampled continuously (eg: the clip is infact an animating roto spline and can be rendered anywhen).
bool
OfxClipInstance::getContinuousSamples() const
{
    EffectInstancePtr effect = getEffectHolder();

    if (!effect) {
        return false;
    }
    if ( isOutput() ) {
        return effect->canRenderContinuously();
    } else {
        EffectInstancePtr associatedNode = getAssociatedNode();

        return associatedNode ? associatedNode->canRenderContinuously() : false;
    }
}

void
OfxClipInstance::getRegionOfDefinitionInternal(OfxTime time,
                                               ViewIdx view,
                                               unsigned int mipmapLevel,
                                               EffectInstancePtr associatedNode,
                                               OfxRectD* ret) const
{
    RotoDrawableItemPtr attachedStroke;
    EffectInstancePtr effect = getEffectHolder();

    if (effect) {
        assert( effect->getNode() );
        attachedStroke = effect->getNode()->getAttachedRotoItem();
    }

    bool inputIsMask = _imp->mask;
    RectD rod;
    if ( attachedStroke && inputIsMask ) {
        effect->getNode()->getPaintStrokeRoD(time, &rod);
        ret->x1 = rod.x1;
        ret->x2 = rod.x2;
        ret->y1 = rod.y1;
        ret->y2 = rod.y2;

        return;
    }
    if (associatedNode) {

        U64 hash;
        bool gotHash = associatedNode->getRenderHash(time, view, &hash);
        (void)gotHash;
        RectD rod;
        RenderScale scale( Image::getScaleFromMipMapLevel(mipmapLevel) );
        StatusEnum st = associatedNode->getRegionOfDefinition_public(hash, time, scale, view, &rod);
        if (st == eStatusFailed) {
            ret->x1 = 0.;
            ret->x2 = 0.;
            ret->y1 = 0.;
            ret->y2 = 0.;
        } else {
            ret->x1 = rod.left();
            ret->x2 = rod.right();
            ret->y1 = rod.bottom();
            ret->y2 = rod.top();
        }
    } else {
        ret->x1 = 0.;
        ret->x2 = 0.;
        ret->y1 = 0.;
        ret->y2 = 0.;
    }
} // OfxClipInstance::getRegionOfDefinitionInternal

// overridden from OFX::Host::ImageEffect::ClipInstance
OfxRectD
OfxClipInstance::getRegionOfDefinition(OfxTime time,
                                       int view) const
{
    OfxRectD rod;
    unsigned int mipmapLevel;
    EffectInstancePtr associatedNode = getAssociatedNode();

    /// The node might be disabled, hence we navigate upstream to find the first non disabled node.
    if (associatedNode) {
        associatedNode = associatedNode->getNearestNonDisabled();
    }
    ///We don't have to do the same kind of navigation if the effect is identity because the effect is supposed to have
    ///the same RoD as the input if it is identity.

    if (!associatedNode) {
        ///Doesn't matter, input is not connected
        mipmapLevel = 0;
    } else {
        ClipDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
        if ( !tls->mipMapLevel.empty() ) {
            mipmapLevel = tls->mipMapLevel.back();
        } else {
            mipmapLevel = 0;
        }
    }
    getRegionOfDefinitionInternal(time, ViewIdx(view), mipmapLevel, associatedNode, &rod);

    return rod;
}

// overridden from OFX::Host::ImageEffect::ClipInstance
/// override this to return the rod on the clip canonical coords!
OfxRectD
OfxClipInstance::getRegionOfDefinition(OfxTime time) const
{
    OfxRectD ret;
    unsigned int mipmapLevel;
    ViewIdx view(0);
    EffectInstancePtr associatedNode = getAssociatedNode();

    /// The node might be disabled, hence we navigate upstream to find the first non disabled node.
    if (associatedNode) {
        associatedNode = associatedNode->getNearestNonDisabled();
    }
    ///We don't have to do the same kind of navigation if the effect is identity because the effect is supposed to have
    ///the same RoD as the input if it is identity.

    if (!associatedNode) {
        ///Doesn't matter, input is not connected
        mipmapLevel = 0;
    } else {
        ClipDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
        if ( !tls->view.empty() ) {
            view = tls->view.back();
        }
        if ( !tls->mipMapLevel.empty() ) {
            mipmapLevel = tls->mipMapLevel.back();
        } else {
            mipmapLevel = 0;
        }
    }
    getRegionOfDefinitionInternal(time, view, mipmapLevel, associatedNode, &ret);

    return ret;
} // getRegionOfDefinition

#ifdef OFX_SUPPORTS_OPENGLRENDER
// overridden from OFX::Host::ImageEffect::ClipInstance
/// override this to fill in the OpenGL texture at the given time.
/// The bounds of the image on the image plane should be
/// 'appropriate', typically the value returned in getRegionsOfInterest
/// on the effect instance. Outside a render call, the optionalBounds should
/// be 'appropriate' for the.
/// If bounds is not null, fetch the indicated section of the canonical image plane.
OFX::Host::ImageEffect::Texture*
OfxClipInstance::loadTexture(OfxTime time,
                             const char *format,
                             const OfxRectD *optionalBounds)
{
    ImageBitDepthEnum depth = eImageBitDepthNone;

    if (format) {
        depth = ofxDepthToNatronDepth( std::string(format) );
    }

    OFX::Host::ImageEffect::Texture* texture = 0;
    if ( !getImagePlaneInternal(time, ViewSpec::current(), optionalBounds, 0 /*plane*/, format ? &depth : 0, 0 /*image*/, &texture) ) {
        return 0;
    }

    return texture;
}

#endif

// overridden from OFX::Host::ImageEffect::ClipInstance
/// override this to fill in the image at the given time.
/// The bounds of the image on the image plane should be
/// 'appropriate', typically the value returned in getRegionsOfInterest
/// on the effect instance. Outside a render call, the optionalBounds should
/// be 'appropriate' for the.
/// If bounds is not null, fetch the indicated section of the canonical image plane.
OFX::Host::ImageEffect::Image*
OfxClipInstance::getImage(OfxTime time,
                          const OfxRectD *optionalBounds)
{
    OFX::Host::ImageEffect::Image* image = 0;

    if ( !getImagePlaneInternal(time, ViewSpec::current(), optionalBounds, 0 /*plane*/, 0 /*texdepth*/, &image, 0 /*tex*/) ) {
        return 0;
    }

    return image;
}

// overridden from OFX::Host::ImageEffect::ClipInstance
OFX::Host::ImageEffect::Image*
OfxClipInstance::getStereoscopicImage(OfxTime time,
                                      int view,
                                      const OfxRectD *optionalBounds)
{
    OFX::Host::ImageEffect::Image* image = 0;

    if ( !getImagePlaneInternal(time, ViewSpec(view), optionalBounds, 0 /*plane*/, 0 /*texdepth*/, &image, 0 /*tex*/) ) {
        return 0;
    }

    return image;
}

// overridden from OFX::Host::ImageEffect::ClipInstance
OFX::Host::ImageEffect::Image*
OfxClipInstance::getImagePlane(OfxTime time,
                               int view,
                               const std::string& plane,
                               const OfxRectD *optionalBounds)
{
    if (time != time) {
        // time is NaN

        return NULL;
    }

    ViewSpec spec;
    // The Foundry Furnace plug-ins pass -1 to the view parameter, we need to deal with it.
    if (view == -1) {
        spec = ViewSpec::current();
    } else {
        spec = ViewIdx(view);
    }

    OFX::Host::ImageEffect::Image* image = 0;
    if ( !getImagePlaneInternal(time, spec, optionalBounds, &plane, 0 /*texdepth*/, &image, 0 /*tex*/) ) {
        return 0;
    }

    return image;
}

bool
OfxClipInstance::getImagePlaneInternal(OfxTime time,
                                       ViewSpec view,
                                       const OfxRectD *optionalBounds,
                                       const std::string* ofxPlane,
                                       const ImageBitDepthEnum* textureDepth,
                                       OFX::Host::ImageEffect::Image** image,
                                       OFX::Host::ImageEffect::Texture** texture)
{
    if (time != time) {
        // time is NaN

        return false;
    }

    if ( isOutput() ) {
        return getOutputImageInternal(ofxPlane, textureDepth, image, texture);
    } else {
        return getInputImageInternal(time, view, optionalBounds, ofxPlane, textureDepth, image, texture);
    }
}

bool
OfxClipInstance::getInputImageInternal(const OfxTime time,
                                       const ViewSpec viewParam,
                                       const OfxRectD *optionalBounds,
                                       const std::string* ofxPlane,
                                       const ImageBitDepthEnum* textureDepth,
                                       OFX::Host::ImageEffect::Image** retImage,
                                       OFX::Host::ImageEffect::Texture** retTexture)
{
    assert( !isOutput() );
    assert( (retImage && !retTexture) || (!retImage && retTexture) );

    ClipDataTLSPtr tls = _imp->tlsData->getTLSData();
    boost::shared_ptr<RenderActionData> renderData;

    //If components param is not set (i.e: the plug-in uses regular clipGetImage call) then figure out the plane from the TLS set in OfxEffectInstance::render
    //otherwise use the param sent by the plug-in call of clipGetImagePlane
    if (tls) {
        if ( !tls->renderData.empty() ) {
            renderData = tls->renderData.back();
            assert(renderData);
        }
    }


    EffectInstancePtr effect = getEffectHolder();
    assert(effect);
    int inputnb = getInputNb();
    const std::string& thisClipComponents = getComponents();

    //If components param is not set (i.e: the plug-in uses regular clipGetImage call) then figure out the plane from the TLS set in OfxEffectInstance::render
    //otherwise use the param sent by the plug-in call of clipGetImagePlane

    //bool isMultiplanar = effect->isMultiPlanar();
    ImageComponents comp;
    if (!ofxPlane) {
        boost::shared_ptr<EffectInstance::ComponentsNeededMap> neededComps;
        effect->getThreadLocalNeededComponents(&neededComps);
        bool foundCompsInTLS = false;
        if (neededComps) {
            EffectInstance::ComponentsNeededMap::iterator found = neededComps->find(inputnb);
            if ( found != neededComps->end() ) {
                if ( found->second.empty() ) {
                    ///We are in the case of a multi-plane effect who did not specify correctly the needed components for an input
                    //fallback on the basic components indicated on the clip
                    //This could be the case for example for the Mask Input
                    comp = ofxComponentsToNatronComponents( thisClipComponents );
                    assert( !comp.isPairedComponents() );
                    foundCompsInTLS = true;
                    //qDebug() << _imp->nodeInstance->getScriptName_mt_safe().c_str() << " didn't specify any needed components via getClipComponents for clip " << getName().c_str();
                } else {
                    comp = found->second.front();
                    foundCompsInTLS = true;
                }
            }
        }

        if (!foundCompsInTLS) {
            ///We are in analysis or the effect does not have any input
            std::bitset<4> processChannels;
            bool isAll;
            bool hasUserComps = effect->getNode()->getSelectedLayer(inputnb, &processChannels, &isAll, &comp);
            if (!hasUserComps) {
                //There's no selector...fallback on the basic components indicated on the clip
                comp = ofxComponentsToNatronComponents( thisClipComponents );
                assert( !comp.isPairedComponents() );
            }
        }
    } else {
        comp = ofxPlaneToNatronPlane(*ofxPlane);
    }

    if (comp.getNumComponents() == 0) {
        return false;
    }
    if (time != time) {
        // time is NaN
        return false;
    }


    unsigned int mipMapLevel = 0;
    // Get mipmaplevel and view from the TLS
#ifdef DEBUG
    if ( !tls || tls->view.empty() ) {
        if ( QThread::currentThread() != qApp->thread() ) {
            qDebug() << effect->getNode()->getScriptName_mt_safe().c_str() << " is trying to call clipGetImage on a thread "
                "not controlled by Natron (probably from the multi-thread suite).\n If you're a developer of that plug-in, please "
                "fix it. Natron is now going to try to recover from that mistake but doing so can yield unpredictable results.";
        }
    }
#endif
    ViewIdx view;
    if (tls) {
        if ( viewParam.isCurrent() ) {
            if ( tls->view.empty() ) {
                view = ViewIdx(0);
            } else {
                view = tls->view.back();
            }
        } else {
            view = ViewIdx( viewParam.value() );
        }

        if ( tls->mipMapLevel.empty() ) {
            mipMapLevel = 0;
        } else {
            mipMapLevel = tls->mipMapLevel.back();
        }
    } else {
        view = ViewIdx( viewParam.value() );
    }

    // If the plug-in is requesting the colour plane, it is expected that we return
    // an image mapped to the clip components
    const bool mapImageToClipPref = !ofxPlane || *ofxPlane == kFnOfxImagePlaneColour;

    //Check if the plug-in already called clipGetImage on this image, in which case we may already have an OfxImage laying around
    //so we try to re-use it.
    if (renderData) {
        for (std::list<OfxImageCommon*>::const_iterator it = renderData->imagesBeingRendered.begin(); it != renderData->imagesBeingRendered.end(); ++it) {
            ImagePtr internalImage = (*it)->getInternalImage();
            if (!internalImage) {
                continue;
            }
            bool sameComponents = (mapImageToClipPref && (*it)->getComponentsString() == thisClipComponents) ||
                                  (!mapImageToClipPref && (*it)->getComponentsString() == *ofxPlane);
            if ( sameComponents && (internalImage->getMipMapLevel() == mipMapLevel) &&
                 ( time == internalImage->getTime() ) &&
                 ( view == internalImage->getKey().getView() ) ) {
                if (retImage) {
                    OfxImage* isImage = dynamic_cast<OfxImage*>(*it);
                    if (isImage) {
                        *retImage = isImage;
                        isImage->addReference();

                        return true;
                    }
                } else if (retTexture) {
                    OfxTexture* isTex = dynamic_cast<OfxTexture*>(*it);
                    if (isTex) {
                        *retTexture = isTex;
                        isTex->addReference();

                        return true;
                    }
                }
            }
        }
    }


    RenderScale renderScale( Image::getScaleFromMipMapLevel(mipMapLevel) );
    RectD bounds;
    if (optionalBounds) {
        bounds.x1 = optionalBounds->x1;
        bounds.y1 = optionalBounds->y1;
        bounds.x2 = optionalBounds->x2;
        bounds.y2 = optionalBounds->y2;
    }

    bool multiPlanar = effect->isMultiPlanar();
    RectI renderWindow;
    boost::shared_ptr<Transform::Matrix3x3> transform;
    ImagePtr image = effect->getImage(inputnb, time, renderScale, view,
                                      optionalBounds ? &bounds : NULL,
                                      &comp,
                                      mapImageToClipPref,
                                      false /*dontUpscale*/,
                                      retTexture != 0 ? eStorageModeGLTex : eStorageModeRAM,
                                      textureDepth,
                                      &renderWindow,
                                      &transform);


    if ( !image || renderWindow.isNull() ) {
        return 0;
    }

    assert(!retTexture || image->getStorageMode() == eStorageModeGLTex);

    std::string components;
    int nComps;
    if (multiPlanar) {
        components = OfxClipInstance::natronsComponentsToOfxComponents( image->getComponents() );
        nComps = image->getComponents().getNumComponents();
    } else {
        components = thisClipComponents;
        ImageComponents natronComps = OfxClipInstance::ofxComponentsToNatronComponents(components);
        nComps = natronComps.getNumComponents();
    }


#ifdef DEBUG
    // This will dump the image as seen from the plug-in
    /*QString filename;
       QTextStream ts(&filename);
       QDateTime now = QDateTime::currentDateTime();
       ts << "img_" << time << "_"  << now.toMSecsSinceEpoch() << ".png";
       appPTR->debugImage(image.get(), renderWindow, filename);*/
#endif

    double par = getAspectRatio();
    OfxImageCommon* retCommon = 0;
    if (retImage) {
        OfxImage* ofxImage = new OfxImage(renderData, image, true, renderWindow, transform, components, nComps, par);
        *retImage = ofxImage;
        retCommon = ofxImage;
    } else if (retTexture) {
        OfxTexture* ofxTex = new OfxTexture(renderData, image, true, renderWindow, transform, components, nComps, par);
        *retTexture = ofxTex;
        retCommon = ofxTex;
    }
    if (renderData) {
        renderData->imagesBeingRendered.push_back(retCommon);
    }

    return true;
} // OfxClipInstance::getInputImageInternal

bool
OfxClipInstance::getOutputImageInternal(const std::string* ofxPlane,
                                        const ImageBitDepthEnum* /*textureDepth*/, // < ignore requested texture depth because internally we use 32bit fp textures, so we offer the highest possible quality anyway.
                                        OFX::Host::ImageEffect::Image** retImage,
                                        OFX::Host::ImageEffect::Texture** retTexture)
{
    ClipDataTLSPtr tls = _imp->tlsData->getTLSData();
    boost::shared_ptr<RenderActionData> renderData;

    assert( (retImage && !retTexture) || (!retImage && retTexture) );

    //If components param is not set (i.e: the plug-in uses regular clipGetImage call) then figure out the plane from the TLS set in OfxEffectInstance::render
    //otherwise use the param sent by the plug-in call of clipGetImagePlane
    if (tls) {
        if ( !tls->renderData.empty() ) {
            renderData = tls->renderData.back();
            assert(renderData);
        }
    }

    EffectInstancePtr effect = getEffectHolder();
    bool isMultiplanar = effect->isMultiPlanar();
    ImageComponents natronPlane;
    if (!ofxPlane) {
        if (renderData) {
            natronPlane = renderData->clipComponents;
        }

        /*
           If the plugin is multi-planar, we are in the situation where it called the regular clipGetImage without a plane in argument
           so the components will not have been set on the TLS hence just use regular components.
         */
        if ( (natronPlane.getNumComponents() == 0) && effect->isMultiPlanar() ) {
            natronPlane = ofxComponentsToNatronComponents( getComponents() );
            assert( !natronPlane.isPairedComponents() );
        }
        assert(natronPlane.getNumComponents() > 0);
    } else {
        natronPlane = ofxPlaneToNatronPlane(*ofxPlane);
    }

    if (natronPlane.getNumComponents() == 0) {
        return false;
    }


    //Look into TLS what planes are being rendered in the render action currently and the render window
    std::map<ImageComponents, EffectInstance::PlaneToRender> outputPlanes;
    RectI renderWindow;
    ImageComponents planeBeingRendered;
    bool ok = effect->getThreadLocalRenderedPlanes(&outputPlanes, &planeBeingRendered, &renderWindow);
    if (!ok) {
        return false;
    }

    ImagePtr outputImage;

    /*
       If the plugin is multiplanar return exactly what it requested.
       Otherwise, hack the clipGetImage and return the plane requested by the user via the interface instead of the colour plane.
     */
    const std::string& layerName = /*multiPlanar ?*/ natronPlane.getLayerName(); // : planeBeingRendered.getLayerName();

    for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
        if (it->first.getLayerName() == layerName) {
            outputImage = it->second.tmpImage;
            break;
        }
    }

    //The output image MAY not exist in the TLS in some cases:
    //e.g: Natron requested Motion.Forward but plug-ins only knows how to render Motion.Forward + Motion.Backward
    //We then just allocate on the fly the plane and cache it.
    if (!outputImage) {
        outputImage = effect->allocateImagePlaneAndSetInThreadLocalStorage(natronPlane);
    }

    //If we don't have it by now then something is really wrong either in TLS or in the plug-in.
    assert(outputImage);
    if (!outputImage) {
        return false;
    }


    //Check if the plug-in already called clipGetImage on this image, in which case we may already have an OfxImage laying around
    //so we try to re-use it.
    if (renderData) {
        for (std::list<OfxImageCommon*>::const_iterator it = renderData->imagesBeingRendered.begin(); it != renderData->imagesBeingRendered.end(); ++it) {
            if ( (*it)->getInternalImage() == outputImage ) {
                if (retImage) {
                    OfxImage* isImage = dynamic_cast<OfxImage*>(*it);
                    if (isImage) {
                        *retImage = isImage;
                        isImage->addReference();

                        return true;
                    }
                } else if (retTexture) {
                    OfxTexture* isTex = dynamic_cast<OfxTexture*>(*it);
                    if (isTex) {
                        *retTexture = isTex;
                        isTex->addReference();

                        return true;
                    }
                }
            }
        }
    }

    //This is the firs time the plug-ins asks for this OfxImage, just allocate it and register it in the TLS
    std::string ofxComponents;
    int nComps;
    if (isMultiplanar) {
        ofxComponents = OfxClipInstance::natronsComponentsToOfxComponents( outputImage->getComponents() );
        nComps = outputImage->getComponents().getNumComponents();
    } else {
        ofxComponents = getComponents();
        ImageComponents natronComps = OfxClipInstance::ofxComponentsToNatronComponents(ofxComponents);
        assert( !natronPlane.isPairedComponents() );
        nComps = natronComps.getNumComponents();
        assert( nComps == (int)outputImage->getComponentsCount() );
        if ( nComps != (int)outputImage->getComponentsCount() ) {
            return false;
        }
    }

    assert(!retTexture || outputImage->getStorageMode() == eStorageModeGLTex);

    //The output clip doesn't have any transform matrix
    double par = getAspectRatio();
    OfxImageCommon* retCommon = 0;
    if (retImage) {
        OfxImage* ret =  new OfxImage(renderData, outputImage, false, renderWindow, boost::shared_ptr<Transform::Matrix3x3>(), ofxComponents, nComps, par);
        *retImage = ret;
        retCommon = ret;
    } else if (retTexture) {
        OfxTexture* ret =  new OfxTexture(renderData, outputImage, false, renderWindow, boost::shared_ptr<Transform::Matrix3x3>(), ofxComponents, nComps, par);
        *retTexture = ret;
        retCommon = ret;
    }

    if (renderData) {
        renderData->imagesBeingRendered.push_back(retCommon);
    }

    return true;
} // OfxClipInstance::getOutputImageInternal

static std::string
natronCustomCompToOfxComp(const ImageComponents &comp)
{
    std::stringstream ss;
    const std::vector<std::string>& channels = comp.getComponentsNames();

    ss << kNatronOfxImageComponentsPlane << comp.getLayerName();
    for (U32 i = 0; i < channels.size(); ++i) {
        ss << kNatronOfxImageComponentsPlaneChannel << channels[i];
    }

    return ss.str();
}

static ImageComponents
ofxCustomCompToNatronComp(const std::string& comp)
{
    std::string layerName;
    std::string compsName;
    std::vector<std::string> channelNames;
    static std::string foundPlaneStr(kNatronOfxImageComponentsPlane);
    static std::string foundChannelStr(kNatronOfxImageComponentsPlaneChannel);
    std::size_t foundPlane = comp.find(foundPlaneStr);

    if (foundPlane == std::string::npos) {
        throw std::runtime_error("Unsupported components type: " + comp);
    }

    std::size_t foundChannel = comp.find( foundChannelStr, foundPlane + foundPlaneStr.size() );
    if (foundChannel == std::string::npos) {
        throw std::runtime_error("Unsupported components type: " + comp);
    }


    for (std::size_t i = foundPlane + foundPlaneStr.size(); i < foundChannel; ++i) {
        layerName.push_back(comp[i]);
    }

    while (foundChannel != std::string::npos) {
        std::size_t nextChannel = comp.find( foundChannelStr, foundChannel + foundChannelStr.size() );
        std::size_t end = nextChannel == std::string::npos ? comp.size() : nextChannel;
        std::string chan;
        for (std::size_t i = foundChannel + foundChannelStr.size(); i < end; ++i) {
            chan.push_back(comp[i]);
        }
        channelNames.push_back(chan);
        compsName.append(chan);

        foundChannel = nextChannel;
    }


    return ImageComponents(layerName, compsName, channelNames);
}

ImageComponents
OfxClipInstance::ofxPlaneToNatronPlane(const std::string& plane)
{
    if (plane == kFnOfxImagePlaneColour) {
        return ofxComponentsToNatronComponents( getComponents() );
    } else if ( (plane == kFnOfxImagePlaneBackwardMotionVector) || (plane == kNatronBackwardMotionVectorsPlaneName) ) {
        return ImageComponents::getBackwardMotionComponents();
    } else if ( (plane == kFnOfxImagePlaneForwardMotionVector) || (plane == kNatronForwardMotionVectorsPlaneName) ) {
        return ImageComponents::getForwardMotionComponents();
    } else if ( (plane == kFnOfxImagePlaneStereoDisparityLeft) || (plane == kNatronDisparityLeftPlaneName) ) {
        return ImageComponents::getDisparityLeftComponents();
    } else if ( (plane == kFnOfxImagePlaneStereoDisparityRight) || (plane == kNatronDisparityRightPlaneName) ) {
        return ImageComponents::getDisparityRightComponents();
    } else {
        try {
            return ofxCustomCompToNatronComp(plane);
        } catch (...) {
            return ImageComponents::getNoneComponents();
        }
    }
}

void
OfxClipInstance::natronsPlaneToOfxPlane(const ImageComponents& plane,
                                        std::list<std::string>* ofxPlanes)
{
    if (plane.getLayerName() == kNatronColorPlaneName) {
        ofxPlanes->push_back(kFnOfxImagePlaneColour);
    } else if ( plane == ImageComponents::getPairedMotionVectors() ) {
        ofxPlanes->push_back(kFnOfxImagePlaneBackwardMotionVector);
        ofxPlanes->push_back(kFnOfxImagePlaneForwardMotionVector);
    } else if ( plane == ImageComponents::getPairedStereoDisparity() ) {
        ofxPlanes->push_back(kFnOfxImagePlaneStereoDisparityLeft);
        ofxPlanes->push_back(kFnOfxImagePlaneStereoDisparityRight);
    } else {
        ofxPlanes->push_back( natronCustomCompToOfxComp(plane) );
    }
}

std::string
OfxClipInstance::natronsComponentsToOfxComponents(const ImageComponents& comp)
{
    if ( comp == ImageComponents::getNoneComponents() ) {
        return kOfxImageComponentNone;
    } else if ( comp == ImageComponents::getAlphaComponents() ) {
        return kOfxImageComponentAlpha;
    } else if ( comp == ImageComponents::getRGBComponents() ) {
        return kOfxImageComponentRGB;
    } else if ( comp == ImageComponents::getRGBAComponents() ) {
        return kOfxImageComponentRGBA;
        /* }
           else if (QString::fromUtf8(comp.getComponentsGlobalName().c_str()).compare(QString::fromUtf8("UV"), Qt::CaseInsensitive) == 0) {
             return kFnOfxImageComponentMotionVectors;
           } else if (QString::fromUtf8(comp.getComponentsGlobalName().c_str()).compare(QString::fromUtf8("XY"), Qt::CaseInsensitive) == 0) {
             return kFnOfxImageComponentStereoDisparity;*/
    } else if ( comp == ImageComponents::getXYComponents() ) {
        return kNatronOfxImageComponentXY;
    } else if ( comp == ImageComponents::getPairedMotionVectors() ) {
        return kFnOfxImageComponentMotionVectors;
    } else if ( comp == ImageComponents::getPairedStereoDisparity() ) {
        return kFnOfxImageComponentStereoDisparity;
    } else {
        return natronCustomCompToOfxComp(comp);
    }
}

ImageComponents
OfxClipInstance::ofxComponentsToNatronComponents(const std::string & comp)
{
    if (comp ==  kOfxImageComponentRGBA) {
        return ImageComponents::getRGBAComponents();
    } else if (comp == kOfxImageComponentAlpha) {
        return ImageComponents::getAlphaComponents();
    } else if (comp == kOfxImageComponentRGB) {
        return ImageComponents::getRGBComponents();
    } else if (comp == kOfxImageComponentNone) {
        return ImageComponents::getNoneComponents();
    } else if (comp == kFnOfxImageComponentMotionVectors) {
        return ImageComponents::getPairedMotionVectors();
    } else if (comp == kFnOfxImageComponentStereoDisparity) {
        return ImageComponents::getPairedStereoDisparity();
    } else if (comp == kNatronOfxImageComponentXY) {
        return ImageComponents::getXYComponents();
    } else {
        try {
            return ofxCustomCompToNatronComp(comp);
        } catch (...) {
        }
    }

    return ImageComponents::getNoneComponents();
}

ImageBitDepthEnum
OfxClipInstance::ofxDepthToNatronDepth(const std::string & depth)
{
    if (depth == kOfxBitDepthByte) {
        return eImageBitDepthByte;
    } else if (depth == kOfxBitDepthShort) {
        return eImageBitDepthShort;
    } else if (depth == kOfxBitDepthHalf) {
        return eImageBitDepthHalf;
    } else if (depth == kOfxBitDepthFloat) {
        return eImageBitDepthFloat;
    } else if (depth == kOfxBitDepthNone) {
        return eImageBitDepthNone;
    } else {
        throw std::runtime_error(depth + ": unsupported bitdepth");
    }
}

const std::string&
OfxClipInstance::natronsDepthToOfxDepth(ImageBitDepthEnum depth)
{
    static const std::string byte(kOfxBitDepthByte);
    static const std::string shrt(kOfxBitDepthShort);
    static const std::string flt(kOfxBitDepthFloat);
    static const std::string hlf(kOfxBitDepthHalf);
    static const std::string none(kOfxBitDepthNone);

    switch (depth) {
    case eImageBitDepthByte:

        return byte;
    case eImageBitDepthShort:

        return shrt;
    case eImageBitDepthHalf:

        return hlf;
    case eImageBitDepthFloat:

        return flt;
    case eImageBitDepthNone:

        return none;
    }

    return none;
}

ImagePremultiplicationEnum
OfxClipInstance::ofxPremultToNatronPremult(const std::string& str)
{
    if (str == kOfxImagePreMultiplied) {
        return eImagePremultiplicationPremultiplied;
    } else if (str == kOfxImageUnPreMultiplied) {
        return eImagePremultiplicationUnPremultiplied;
    } else if (str == kOfxImageOpaque) {
        return eImagePremultiplicationOpaque;
    } else {
        assert(false);

        return eImagePremultiplicationPremultiplied;
    }
}

const std::string&
OfxClipInstance::natronsPremultToOfxPremult(ImagePremultiplicationEnum premult)
{
    static const std::string prem(kOfxImagePreMultiplied);
    static const std::string unprem(kOfxImageUnPreMultiplied);
    static const std::string opq(kOfxImageOpaque);

    switch (premult) {
    case eImagePremultiplicationPremultiplied:

        return prem;
    case eImagePremultiplicationUnPremultiplied:

        return unprem;
    case eImagePremultiplicationOpaque:

        return opq;
    }

    return prem;
}

ImageFieldingOrderEnum
OfxClipInstance::ofxFieldingToNatronFielding(const std::string& fielding)
{
    if (fielding == kOfxImageFieldNone) {
        return eImageFieldingOrderNone;
    } else if (fielding == kOfxImageFieldLower) {
        return eImageFieldingOrderLower;
    } else if (fielding == kOfxImageFieldUpper) {
        return eImageFieldingOrderUpper;
    } else {
        assert(false);
        throw std::invalid_argument("Unknown fielding " + fielding);
    }
}

const std::string&
OfxClipInstance::natronsFieldingToOfxFielding(ImageFieldingOrderEnum fielding)
{
    static const std::string noFielding(kOfxImageFieldNone);
    static const std::string upperFielding(kOfxImageFieldUpper);
    static const std::string lowerFielding(kOfxImageFieldLower);

    switch (fielding) {
    case eImageFieldingOrderNone:

        return noFielding;
    case eImageFieldingOrderUpper:

        return upperFielding;
    case eImageFieldingOrderLower:

        return lowerFielding;
    }

    return noFielding;
}

struct OfxImageCommonPrivate
{
    OFX::Host::ImageEffect::ImageBase* ofxImageBase;
    ImagePtr natronImage;
    boost::shared_ptr<GenericAccess> access;
    boost::shared_ptr<OfxClipInstance::RenderActionData> tls;
    std::string components;
    boost::scoped_ptr<RamBuffer<unsigned char> > localBuffer;

    OfxImageCommonPrivate(OFX::Host::ImageEffect::ImageBase* ofxImageBase,
                          const ImagePtr& image,
                          const boost::shared_ptr<OfxClipInstance::RenderActionData>& tls)
        : ofxImageBase(ofxImageBase)
        , natronImage(image)
        , access()
        , tls(tls)
        , components()
        , localBuffer()
    {
    }
};

ImagePtr
OfxImageCommon::getInternalImage() const
{
    return _imp->natronImage;
}

const std::string&
OfxImageCommon::getComponentsString() const
{
    return _imp->components;
}

OfxImageCommon::OfxImageCommon(OFX::Host::ImageEffect::ImageBase* ofxImageBase,
                               const boost::shared_ptr<OfxClipInstance::RenderActionData>& renderData,
                               const boost::shared_ptr<NATRON_NAMESPACE::Image>& internalImage,
                               bool isSrcImage,
                               const RectI& renderWindow,
                               const boost::shared_ptr<Transform::Matrix3x3>& mat,
                               const std::string& components,
                               int nComps,
                               double par)
    : _imp( new OfxImageCommonPrivate(ofxImageBase, internalImage, renderData) )
{
    _imp->components = components;

    assert(internalImage);

    unsigned int mipMapLevel = internalImage->getMipMapLevel();
    RenderScale scale( NATRON_NAMESPACE::Image::getScaleFromMipMapLevel(mipMapLevel) );
    ofxImageBase->setDoubleProperty(kOfxImageEffectPropRenderScale, scale.x, 0);
    ofxImageBase->setDoubleProperty(kOfxImageEffectPropRenderScale, scale.y, 1);

    StorageModeEnum storage = internalImage->getStorageMode();
    if (storage == eStorageModeGLTex) {
        ofxImageBase->setIntProperty( kOfxImageEffectPropOpenGLTextureTarget, internalImage->getGLTextureTarget() );
        ofxImageBase->setIntProperty( kOfxImageEffectPropOpenGLTextureIndex, internalImage->getGLTextureID() );
    }

    const RectD & rod = internalImage->getRoD(); // Not the OFX RoD!!! Image::getRoD() is in *CANONICAL* coordinates

    // The bounds of the image at the moment we peak the rowBytes and the internal buffer pointer.
    // Note that when the ReadAccess, or WriteAccess object is released, the image may be resized afterwards (only bigger)
    RectI pluginsSeenBounds;

    int dataSizeOf = getSizeOfForBitDepth( internalImage->getBitDepth() );


    if (isSrcImage) {
        // Some plug-ins need a local version of the input image because they modify it (e.g: ReMap). This is out of spec
        // and if it does so, it may modify the cached output of the node from which this input image comes from.
        // To circumvent this, we copy the source image into a local temporary buffer only used by the plug-in which is released
        // when this OfxImage is destroyed. By default this local copy is deactivated, to activate it, the user has to go
        // in the preferences and check "Use input image copy for plug-ins rendering"
        const bool copySrcToPluginLocalData = appPTR->isCopyInputImageForPluginRenderEnabled();
        boost::shared_ptr<NATRON_NAMESPACE::Image::ReadAccess> access( new NATRON_NAMESPACE::Image::ReadAccess( internalImage.get() ) );

        // data ptr
        const RectI bounds = internalImage->getBounds();
        renderWindow.intersect(bounds, &pluginsSeenBounds);
        const std::size_t srcRowSize = bounds.width() * nComps  * dataSizeOf;

        // row bytes
        ofxImageBase->setIntProperty(kOfxImagePropRowBytes, srcRowSize);

        if (storage == eStorageModeGLTex) {
            _imp->access = access;
        } else {


            if (!copySrcToPluginLocalData) {
                const unsigned char* ptr = access->pixelAt( pluginsSeenBounds.left(), pluginsSeenBounds.bottom() );
                assert(ptr);
                ofxImageBase->setPointerProperty( kOfxImagePropData, const_cast<unsigned char*>(ptr) );
                _imp->access = access;
            } else {
                std::size_t dstRowSize = pluginsSeenBounds.width() * dataSizeOf * nComps;
                std::size_t bufferSize = dstRowSize * pluginsSeenBounds.height();
                _imp->localBuffer.reset( new RamBuffer<unsigned char>() );
                _imp->localBuffer->resize(bufferSize);
                unsigned char* localBufferData = _imp->localBuffer->getData();
                assert(localBufferData);
                if (localBufferData) {
                    unsigned char* dstPixels = localBufferData;
                    const unsigned char* srcPix = access->pixelAt( pluginsSeenBounds.left(), pluginsSeenBounds.bottom() );
                    assert(srcPix);
                    for (int y = pluginsSeenBounds.y1; y < pluginsSeenBounds.y2; ++y,
                         dstPixels += dstRowSize,
                         srcPix += srcRowSize) {

                        memcpy(dstPixels, srcPix, dstRowSize);
                    }

                }
                ofxImageBase->setPointerProperty( kOfxImagePropData, localBufferData );
                // we changed row bytes
                ofxImageBase->setIntProperty(kOfxImagePropRowBytes, dstRowSize);
            }
        }
    } else {
        const RectI bounds = internalImage->getBounds();
        const std::size_t srcRowSize = bounds.width() * nComps  * dataSizeOf;

        // row bytes
        ofxImageBase->setIntProperty(kOfxImagePropRowBytes, srcRowSize);
        
        boost::shared_ptr<NATRON_NAMESPACE::Image::WriteAccess> access( new NATRON_NAMESPACE::Image::WriteAccess( internalImage.get() ) );

        // data ptr
        renderWindow.intersect(bounds, &pluginsSeenBounds);

        if (storage != eStorageModeGLTex) {
            unsigned char* ptr = access->pixelAt( pluginsSeenBounds.left(), pluginsSeenBounds.bottom() );
            assert(ptr);
            ofxImageBase->setPointerProperty( kOfxImagePropData, ptr);
        }

        _imp->access = access;
    } // isSrcImage

    ///Do not activate this assert! The render window passed to renderRoI can be bigger than the actual RoD of the effect
    ///in which case it is just clipped to the RoD.
    //assert(bounds.contains(renderWindow));

    ///We set the render window that was given to the render thread instead of the actual bounds of the image
    ///so we're sure the plug-in doesn't attempt to access outside pixels.
    ofxImageBase->setIntProperty(kOfxImagePropBounds, pluginsSeenBounds.left(), 0);
    ofxImageBase->setIntProperty(kOfxImagePropBounds, pluginsSeenBounds.bottom(), 1);
    ofxImageBase->setIntProperty(kOfxImagePropBounds, pluginsSeenBounds.right(), 2);
    ofxImageBase->setIntProperty(kOfxImagePropBounds, pluginsSeenBounds.top(), 3);

    // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImagePropRegionOfDefinition
    // " An image's region of definition, in *PixelCoordinates,* is the full frame area of the image plane that the image covers."
    // Image::getRoD() is in *CANONICAL* coordinates
    // OFX::Image RoD is in *PIXEL* coordinates
    RectI pixelRod;
    rod.toPixelEnclosing(mipMapLevel, internalImage->getPixelAspectRatio(), &pixelRod);
    ofxImageBase->setIntProperty(kOfxImagePropRegionOfDefinition, pixelRod.left(), 0);
    ofxImageBase->setIntProperty(kOfxImagePropRegionOfDefinition, pixelRod.bottom(), 1);
    ofxImageBase->setIntProperty(kOfxImagePropRegionOfDefinition, pixelRod.right(), 2);
    ofxImageBase->setIntProperty(kOfxImagePropRegionOfDefinition, pixelRod.top(), 3);

    //pluginsSeenBounds must be contained in pixelRod
    assert( pluginsSeenBounds.left() >= pixelRod.left() && pluginsSeenBounds.right() <= pixelRod.right() &&
            pluginsSeenBounds.bottom() >= pixelRod.bottom() && pluginsSeenBounds.top() <= pixelRod.top() );


    ofxImageBase->setStringProperty( kOfxImageEffectPropComponents, components);
    ofxImageBase->setStringProperty( kOfxImageEffectPropPixelDepth, OfxClipInstance::natronsDepthToOfxDepth( internalImage->getBitDepth() ) );
    ofxImageBase->setStringProperty( kOfxImageEffectPropPreMultiplication, OfxClipInstance::natronsPremultToOfxPremult( internalImage->getPremultiplication() ) );
    ofxImageBase->setStringProperty( kOfxImagePropField, OfxClipInstance::natronsFieldingToOfxFielding( internalImage->getFieldingOrder() ) );
    ofxImageBase->setStringProperty( kOfxImagePropUniqueIdentifier, QString::number(internalImage->getHashKey(), 16).toStdString() );
    ofxImageBase->setDoubleProperty( kOfxImagePropPixelAspectRatio, par );

    //Attach the transform matrix if any
    if (mat) {
        ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->a, 0);
        ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->b, 1);
        ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->c, 2);

        ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->d, 3);
        ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->e, 4);
        ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->f, 5);

        ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->g, 6);
        ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->h, 7);
        ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, mat->i, 8);
    } else {
        for (int i = 0; i < 9; ++i) {
            ofxImageBase->setDoubleProperty(kFnOfxPropMatrix2D, 0., i);
        }
    }
}

OfxImageCommon::~OfxImageCommon()
{
    if (_imp->tls) {
        std::list<OfxImageCommon*>::iterator found = std::find(_imp->tls->imagesBeingRendered.begin(), _imp->tls->imagesBeingRendered.end(), this);
        if ( found != _imp->tls->imagesBeingRendered.end() ) {
            _imp->tls->imagesBeingRendered.erase(found);
        }
    }
}

int
OfxClipInstance::getInputNb() const
{
    if (_isOutput) {
        return -1;
    }

    return _imp->nodeInstance.lock()->getClipInputNumber(this);
}

EffectInstancePtr
OfxClipInstance::getEffectHolder() const
{
    OfxEffectInstancePtr effect = _imp->nodeInstance.lock();

    if (!effect) {
        return effect;
    }
    if ( effect->isReader() ) {
        NodePtr ioContainer = effect->getNode()->getIOContainer();
        if (ioContainer) {
            return ioContainer->getEffectInstance();
        }
    }
    return effect;
}

EffectInstancePtr
OfxClipInstance::getAssociatedNode() const
{
    EffectInstancePtr effect = getEffectHolder();

    assert(effect);
    if (_isOutput) {
        return effect;
    } else {
        ImageComponents comps;
        NodePtr maskInput;
        int inputNb = getInputNb();
        effect->getNode()->getMaskChannel(inputNb, &comps, &maskInput);
        if (maskInput) {
            return maskInput->getEffectInstance();
        }

        if (!maskInput) {
            return effect->getInput( getInputNb() );
        } else {
            return maskInput->getEffectInstance();
        }
    }
}

void
OfxClipInstance::setClipTLS(ViewIdx view,
                            unsigned int mipmapLevel,
                            const ImageComponents& components)
{
    ClipDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();

    assert(tls);
    tls->view.push_back(view);
    tls->mipMapLevel.push_back(mipmapLevel);
    boost::shared_ptr<RenderActionData> d( new RenderActionData() );
    d->clipComponents = components;
    tls->renderData.push_back(d);
}

void
OfxClipInstance::invalidateClipTLS()
{
    ClipDataTLSPtr tls = _imp->tlsData->getTLSData();

    assert(tls);
    assert( !tls->view.empty() );
    tls->view.pop_back();
    assert( !tls->mipMapLevel.empty() );
    tls->mipMapLevel.pop_back();
    assert( !tls->renderData.empty() );
    tls->renderData.pop_back();
}

const std::string &
OfxClipInstance::findSupportedComp(const std::string &s) const
{
    static const std::string none(kOfxImageComponentNone);
    static const std::string rgba(kOfxImageComponentRGBA);
    static const std::string rgb(kOfxImageComponentRGB);
    static const std::string alpha(kOfxImageComponentAlpha);
    static const std::string motion(kFnOfxImageComponentMotionVectors);
    static const std::string disparity(kFnOfxImageComponentStereoDisparity);
    static const std::string xy(kNatronOfxImageComponentXY);

    /// is it there
    if ( isSupportedComponent(s) ) {
        return s;
    }

    if (s == xy) {
        if ( isSupportedComponent(motion) ) {
            return motion;
        } else if ( isSupportedComponent(disparity) ) {
            return disparity;
        } else if ( isSupportedComponent(rgb) ) {
            return rgb;
        } else if ( isSupportedComponent(rgba) ) {
            return rgba;
        } else if ( isSupportedComponent(alpha) ) {
            return alpha;
        }
    } else if (s == rgba) {
        if ( isSupportedComponent(rgb) ) {
            return rgb;
        }
        if ( isSupportedComponent(alpha) ) {
            return alpha;
        }
    } else if (s == alpha) {
        if ( isSupportedComponent(rgba) ) {
            return rgba;
        }
        if ( isSupportedComponent(rgb) ) {
            return rgb;
        }
    } else if (s == rgb) {
        if ( isSupportedComponent(rgba) ) {
            return rgba;
        }
        if ( isSupportedComponent(alpha) ) {
            return alpha;
        }
    } else if (s == motion) {
        if ( isSupportedComponent(xy) ) {
            return xy;
        } else if ( isSupportedComponent(disparity) ) {
            return disparity;
        } else if ( isSupportedComponent(rgb) ) {
            return rgb;
        } else if ( isSupportedComponent(rgba) ) {
            return rgba;
        } else if ( isSupportedComponent(alpha) ) {
            return alpha;
        }
    } else if (s == disparity) {
        if ( isSupportedComponent(xy) ) {
            return xy;
        } else if ( isSupportedComponent(disparity) ) {
            return disparity;
        } else if ( isSupportedComponent(rgb) ) {
            return rgb;
        } else if ( isSupportedComponent(rgba) ) {
            return rgba;
        } else if ( isSupportedComponent(alpha) ) {
            return alpha;
        }
    }

    /// wierd, must be some custom bit , if only one, choose that, otherwise no idea
    /// how to map, you need to derive to do so.
    const std::vector<std::string> &supportedComps = getSupportedComponents();
    if (supportedComps.size() == 1) {
        return supportedComps[0];
    }

    return none;
} // OfxClipInstance::findSupportedComp

NATRON_NAMESPACE_EXIT;

