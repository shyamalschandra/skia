/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm.h"

#include "Resources.h"
#include "SkCanvas.h"
#include "SkData.h"
#include "SkDiscardableMemoryPool.h"
#include "SkDiscardablePixelRef.h"
#include "SkImageDecoder.h"
#include "SkImageGeneratorPriv.h"
#include "SkOSFile.h"
#include "SkStream.h"

namespace skiagm {

/**
 *  Draw a PNG created by SkBitmapFactory.
 */
class FactoryGM : public GM {
public:
    FactoryGM() {}

protected:
    void onOnceBeforeDraw() SK_OVERRIDE {
        // Copyright-free file from http://openclipart.org/detail/29213/paper-plane-by-ddoo
        SkString pngFilename = GetResourcePath("plane.png");
        SkAutoDataUnref data(SkData::NewFromFileName(pngFilename.c_str()));
        if (data.get()) {
            // Create a cache which will boot the pixels out anytime the
            // bitmap is unlocked.
            SkAutoTUnref<SkDiscardableMemoryPool> pool(
                SkDiscardableMemoryPool::Create(1));
            SkAssertResult(SkInstallDiscardablePixelRef(SkImageGenerator::NewFromData(data),
                                                        &fBitmap, pool));
        }
    }

    virtual SkString onShortName() {
        return SkString("factory");
    }

    virtual SkISize onISize() {
        return SkISize::Make(640, 480);
    }

    virtual void onDraw(SkCanvas* canvas) {
        canvas->drawBitmap(fBitmap, 0, 0);
    }

private:
    SkBitmap fBitmap;

    typedef GM INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static GM* MyFactory(void*) { return new FactoryGM; }
static GMRegistry reg(MyFactory);

}  // namespace skiagm
