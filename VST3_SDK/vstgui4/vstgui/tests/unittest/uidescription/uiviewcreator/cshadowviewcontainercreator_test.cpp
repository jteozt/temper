//-----------------------------------------------------------------------------
// VST Plug-Ins SDK
// VSTGUI: Graphical User Interface Framework for VST plugins
//
// Version 4.3
//
//-----------------------------------------------------------------------------
// VSTGUI LICENSE
// (c) 2015, Steinberg Media Technologies, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "../../unittests.h"
#include "../../../../uidescription/uiviewfactory.h"
#include "../../../../uidescription/uiattributes.h"
#include "../../../../uidescription/detail/uiviewcreatorattributes.h"
#include "../../../../lib/cshadowviewcontainer.h"
#include "../../../../lib/cstring.h"
#include "helpers.h"

namespace VSTGUI {
using namespace UIViewCreator;

TESTCASE(CShadowViewContainerCreatorTest,

	TEST(shadowIntensity,
		DummyUIDescription uidesc;
		testAttribute<CShadowViewContainer>(kCShadowViewContainer, kAttrShadowIntensity, 0.5, &uidesc, [] (CShadowViewContainer* v) {
			return v->getShadowIntensity () == 0.5f;
		});
	);

	TEST(shadowBlurSize,
		DummyUIDescription uidesc;
		testAttribute<CShadowViewContainer>(kCShadowViewContainer, kAttrShadowBlurSize, 0.5, &uidesc, [] (CShadowViewContainer* v) {
			return v->getShadowBlurSize () == 0.5f;
		});
	);

	TEST(shadowOffset,
		DummyUIDescription uidesc;
		CPoint p (20, 20);
		testAttribute<CShadowViewContainer>(kCShadowViewContainer, kAttrShadowOffset, p, &uidesc, [&] (CShadowViewContainer* v) {
			return v->getShadowOffset () == p;
		});
	);
	
	TEST(shadowBlurSizeMinMax,
		DummyUIDescription uidesc;
		testMinMaxValues(kCShadowViewContainer, kAttrShadowBlurSize, &uidesc, 0.8, 20);
	);

	TEST(shadowIntensityMinMax,
		DummyUIDescription uidesc;
		testMinMaxValues(kCShadowViewContainer, kAttrShadowIntensity, &uidesc, 0., 1.);
	);
);

} // VSTGUI
