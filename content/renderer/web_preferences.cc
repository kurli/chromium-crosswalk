// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/renderer/web_preferences.h"

#include "base/strings/utf_string_conversions.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebNetworkStateNotifier.h"
#include "third_party/WebKit/public/web/WebRuntimeFeatures.h"
#include "third_party/WebKit/public/web/WebSettings.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "third_party/icu/source/common/unicode/uchar.h"
#include "third_party/icu/source/common/unicode/uscript.h"
#include "webkit/common/webpreferences.h"

using WebKit::WebNetworkStateNotifier;
using WebKit::WebRuntimeFeatures;
using WebKit::WebSettings;
using WebKit::WebString;
using WebKit::WebURL;
using WebKit::WebView;

namespace content {

namespace {

typedef void (*SetFontFamilyWrapper)(WebKit::WebSettings*,
                                     const base::string16&,
                                     UScriptCode);

void setStandardFontFamilyWrapper(WebSettings* settings,
                                  const base::string16& font,
                                  UScriptCode script) {
  settings->setStandardFontFamily(font, script);
}

void setFixedFontFamilyWrapper(WebSettings* settings,
                               const base::string16& font,
                               UScriptCode script) {
  settings->setFixedFontFamily(font, script);
}

void setSerifFontFamilyWrapper(WebSettings* settings,
                               const base::string16& font,
                               UScriptCode script) {
  settings->setSerifFontFamily(font, script);
}

void setSansSerifFontFamilyWrapper(WebSettings* settings,
                                   const base::string16& font,
                                   UScriptCode script) {
  settings->setSansSerifFontFamily(font, script);
}

void setCursiveFontFamilyWrapper(WebSettings* settings,
                                 const base::string16& font,
                                 UScriptCode script) {
  settings->setCursiveFontFamily(font, script);
}

void setFantasyFontFamilyWrapper(WebSettings* settings,
                                 const base::string16& font,
                                 UScriptCode script) {
  settings->setFantasyFontFamily(font, script);
}

void setPictographFontFamilyWrapper(WebSettings* settings,
                                    const base::string16& font,
                                    UScriptCode script) {
  settings->setPictographFontFamily(font, script);
}

// If |scriptCode| is a member of a family of "similar" script codes, returns
// the script code in that family that is used by WebKit for font selection
// purposes.  For example, USCRIPT_KATAKANA_OR_HIRAGANA and USCRIPT_JAPANESE are
// considered equivalent for the purposes of font selection.  WebKit uses the
// script code USCRIPT_KATAKANA_OR_HIRAGANA.  So, if |scriptCode| is
// USCRIPT_JAPANESE, the function returns USCRIPT_KATAKANA_OR_HIRAGANA.  WebKit
// uses different scripts than the ones in Chrome pref names because the version
// of ICU included on certain ports does not have some of the newer scripts.  If
// |scriptCode| is not a member of such a family, returns |scriptCode|.
UScriptCode GetScriptForWebSettings(UScriptCode scriptCode) {
  switch (scriptCode) {
  case USCRIPT_HIRAGANA:
  case USCRIPT_KATAKANA:
  case USCRIPT_JAPANESE:
    return USCRIPT_KATAKANA_OR_HIRAGANA;
  case USCRIPT_KOREAN:
    return USCRIPT_HANGUL;
  default:
    return scriptCode;
  }
}

void ApplyFontsFromMap(const webkit_glue::ScriptFontFamilyMap& map,
                       SetFontFamilyWrapper setter,
                       WebSettings* settings) {
  for (webkit_glue::ScriptFontFamilyMap::const_iterator it = map.begin();
       it != map.end();
       ++it) {
    int32 script = u_getPropertyValueEnum(UCHAR_SCRIPT, (it->first).c_str());
    if (script >= 0 && script < USCRIPT_CODE_LIMIT) {
      UScriptCode code = static_cast<UScriptCode>(script);
      (*setter)(settings, it->second, GetScriptForWebSettings(code));
    }
  }
}

}  // namespace

void ApplyWebPreferences(const WebPreferences& prefs, WebView* web_view) {
  WebSettings* settings = web_view->settings();
  ApplyFontsFromMap(prefs.standard_font_family_map,
                    setStandardFontFamilyWrapper, settings);
  ApplyFontsFromMap(prefs.fixed_font_family_map,
                    setFixedFontFamilyWrapper, settings);
  ApplyFontsFromMap(prefs.serif_font_family_map,
                    setSerifFontFamilyWrapper, settings);
  ApplyFontsFromMap(prefs.sans_serif_font_family_map,
                    setSansSerifFontFamilyWrapper, settings);
  ApplyFontsFromMap(prefs.cursive_font_family_map,
                    setCursiveFontFamilyWrapper, settings);
  ApplyFontsFromMap(prefs.fantasy_font_family_map,
                    setFantasyFontFamilyWrapper, settings);
  ApplyFontsFromMap(prefs.pictograph_font_family_map,
                    setPictographFontFamilyWrapper, settings);
  settings->setDefaultFontSize(prefs.default_font_size);
  settings->setDefaultFixedFontSize(prefs.default_fixed_font_size);
  settings->setMinimumFontSize(prefs.minimum_font_size);
  settings->setMinimumLogicalFontSize(prefs.minimum_logical_font_size);
  settings->setDefaultTextEncodingName(ASCIIToUTF16(prefs.default_encoding));
  settings->setJavaScriptEnabled(prefs.javascript_enabled);
  settings->setWebSecurityEnabled(prefs.web_security_enabled);
  settings->setJavaScriptCanOpenWindowsAutomatically(
      prefs.javascript_can_open_windows_automatically);
  settings->setLoadsImagesAutomatically(prefs.loads_images_automatically);
  settings->setImagesEnabled(prefs.images_enabled);
  settings->setPluginsEnabled(prefs.plugins_enabled);
  settings->setDOMPasteAllowed(prefs.dom_paste_enabled);
  settings->setNeedsSiteSpecificQuirks(prefs.site_specific_quirks_enabled);
  settings->setShrinksStandaloneImagesToFit(
      prefs.shrinks_standalone_images_to_fit);
  settings->setUsesEncodingDetector(prefs.uses_universal_detector);
  settings->setTextAreasAreResizable(prefs.text_areas_are_resizable);
  settings->setAllowScriptsToCloseWindows(prefs.allow_scripts_to_close_windows);
  if (prefs.user_style_sheet_enabled)
    settings->setUserStyleSheetLocation(prefs.user_style_sheet_location);
  else
    settings->setUserStyleSheetLocation(WebURL());
  settings->setAuthorAndUserStylesEnabled(prefs.author_and_user_styles_enabled);
  settings->setDownloadableBinaryFontsEnabled(prefs.remote_fonts_enabled);
  settings->setJavaScriptCanAccessClipboard(
      prefs.javascript_can_access_clipboard);
  settings->setXSSAuditorEnabled(prefs.xss_auditor_enabled);
  settings->setDNSPrefetchingEnabled(prefs.dns_prefetching_enabled);
  settings->setLocalStorageEnabled(prefs.local_storage_enabled);
  settings->setSyncXHRInDocumentsEnabled(prefs.sync_xhr_in_documents_enabled);
  WebRuntimeFeatures::enableDatabase(prefs.databases_enabled);
  settings->setOfflineWebApplicationCacheEnabled(
      prefs.application_cache_enabled);
  settings->setCaretBrowsingEnabled(prefs.caret_browsing_enabled);
  settings->setHyperlinkAuditingEnabled(prefs.hyperlink_auditing_enabled);
  settings->setCookieEnabled(prefs.cookie_enabled);

  // This setting affects the behavior of links in an editable region:
  // clicking the link should select it rather than navigate to it.
  // Safari uses the same default. It is unlikley an embedder would want to
  // change this, since it would break existing rich text editors.
  settings->setEditableLinkBehaviorNeverLive();

  settings->setJavaEnabled(prefs.java_enabled);

  // By default, allow_universal_access_from_file_urls is set to false and thus
  // we mitigate attacks from local HTML files by not granting file:// URLs
  // universal access. Only test shell will enable this.
  settings->setAllowUniversalAccessFromFileURLs(
      prefs.allow_universal_access_from_file_urls);
  settings->setAllowFileAccessFromFileURLs(
      prefs.allow_file_access_from_file_urls);

  // Enable the web audio API if requested on the command line.
  settings->setWebAudioEnabled(prefs.webaudio_enabled);

  // Enable experimental WebGL support if requested on command line
  // and support is compiled in.
  settings->setExperimentalWebGLEnabled(prefs.experimental_webgl_enabled);

  // Disable GL multisampling if requested on command line.
  settings->setOpenGLMultisamplingEnabled(prefs.gl_multisampling_enabled);

  // Enable privileged WebGL extensions for Chrome extensions or if requested
  // on command line.
  settings->setPrivilegedWebGLExtensionsEnabled(
      prefs.privileged_webgl_extensions_enabled);

  // Enable WebGL errors to the JS console if requested.
  settings->setWebGLErrorsToConsoleEnabled(
      prefs.webgl_errors_to_console_enabled);

  // Enables accelerated compositing for overflow scroll.
  settings->setAcceleratedCompositingForOverflowScrollEnabled(
      prefs.accelerated_compositing_for_overflow_scroll_enabled);

  // Enables accelerated compositing for scrollable frames if requested on
  // command line.
  settings->setAcceleratedCompositingForScrollableFramesEnabled(
      prefs.accelerated_compositing_for_scrollable_frames_enabled);

  // Enables composited scrolling for frames if requested on command line.
  settings->setCompositedScrollingForFramesEnabled(
      prefs.composited_scrolling_for_frames_enabled);

  // Uses the mock theme engine for scrollbars.
  settings->setMockScrollbarsEnabled(prefs.mock_scrollbars_enabled);

  settings->setThreadedHTMLParser(prefs.threaded_html_parser);

  // Display visualization of what has changed on the screen using an
  // overlay of rects, if requested on the command line.
  settings->setShowPaintRects(prefs.show_paint_rects);

  // Enable gpu-accelerated compositing if requested on the command line.
  settings->setAcceleratedCompositingEnabled(
      prefs.accelerated_compositing_enabled);

  // Enable gpu-accelerated 2d canvas if requested on the command line.
  settings->setAccelerated2dCanvasEnabled(prefs.accelerated_2d_canvas_enabled);

  settings->setMinimumAccelerated2dCanvasSize(
      prefs.minimum_accelerated_2d_canvas_size);

  // Disable antialiasing for 2d canvas if requested on the command line.
  settings->setAntialiased2dCanvasEnabled(
      !prefs.antialiased_2d_canvas_disabled);

  // Enable gpu-accelerated filters if requested on the command line.
  settings->setAcceleratedFiltersEnabled(prefs.accelerated_filters_enabled);

  // Enable gesture tap highlight if requested on the command line.
  settings->setGestureTapHighlightEnabled(prefs.gesture_tap_highlight_enabled);

  // Enabling accelerated layers from the command line enabled accelerated
  // 3D CSS, Video, and Animations.
  settings->setAcceleratedCompositingFor3DTransformsEnabled(
      prefs.accelerated_compositing_for_3d_transforms_enabled);
  settings->setAcceleratedCompositingForVideoEnabled(
      prefs.accelerated_compositing_for_video_enabled);
  settings->setAcceleratedCompositingForAnimationEnabled(
      prefs.accelerated_compositing_for_animation_enabled);

  // Enabling accelerated plugins if specified from the command line.
  settings->setAcceleratedCompositingForPluginsEnabled(
      prefs.accelerated_compositing_for_plugins_enabled);

  // WebGL and accelerated 2D canvas are always gpu composited.
  settings->setAcceleratedCompositingForCanvasEnabled(
      prefs.experimental_webgl_enabled || prefs.accelerated_2d_canvas_enabled);

  // Enable memory info reporting to page if requested on the command line.
  settings->setMemoryInfoEnabled(prefs.memory_info_enabled);

  settings->setAsynchronousSpellCheckingEnabled(
      prefs.asynchronous_spell_checking_enabled);
  settings->setUnifiedTextCheckerEnabled(prefs.unified_textchecker_enabled);

  for (webkit_glue::WebInspectorPreferences::const_iterator it =
           prefs.inspector_settings.begin();
       it != prefs.inspector_settings.end();
       ++it) {
    web_view->setInspectorSetting(WebString::fromUTF8(it->first),
                                  WebString::fromUTF8(it->second));
  }

  // Tabs to link is not part of the settings. WebCore calls
  // ChromeClient::tabsToLinks which is part of the glue code.
  web_view->setTabsToLinks(prefs.tabs_to_links);

  settings->setFullScreenEnabled(prefs.fullscreen_enabled);
  settings->setAllowDisplayOfInsecureContent(
      prefs.allow_displaying_insecure_content);
  settings->setAllowRunningOfInsecureContent(
      prefs.allow_running_insecure_content);
  settings->setPasswordEchoEnabled(prefs.password_echo_enabled);
  settings->setShouldPrintBackgrounds(prefs.should_print_backgrounds);
  settings->setEnableScrollAnimator(prefs.enable_scroll_animator);
  settings->setVisualWordMovementEnabled(prefs.visual_word_movement_enabled);

  settings->setCSSStickyPositionEnabled(prefs.css_sticky_position_enabled);
  settings->setExperimentalCSSCustomFilterEnabled(prefs.css_shaders_enabled);
  settings->setRegionBasedColumnsEnabled(prefs.region_based_columns_enabled);

  WebRuntimeFeatures::enableLazyLayout(prefs.lazy_layout_enabled);
  WebRuntimeFeatures::enableTouch(prefs.touch_enabled);
  settings->setDeviceSupportsTouch(prefs.device_supports_touch);
  settings->setDeviceSupportsMouse(prefs.device_supports_mouse);
  settings->setEnableTouchAdjustment(prefs.touch_adjustment_enabled);

  settings->setFixedPositionCreatesStackingContext(
      prefs.fixed_position_creates_stacking_context);

  settings->setDeferredImageDecodingEnabled(
      prefs.deferred_image_decoding_enabled);
  settings->setShouldRespectImageOrientation(
      prefs.should_respect_image_orientation);

  settings->setUnsafePluginPastingEnabled(false);
  settings->setEditingBehavior(
      static_cast<WebSettings::EditingBehavior>(prefs.editing_behavior));

  settings->setSupportsMultipleWindows(prefs.supports_multiple_windows);

  settings->setViewportEnabled(prefs.viewport_enabled);
  settings->setInitializeAtMinimumPageScale(
      prefs.initialize_at_minimum_page_scale);

  settings->setSmartInsertDeleteEnabled(prefs.smart_insert_delete_enabled);

  settings->setSpatialNavigationEnabled(prefs.spatial_navigation_enabled);

  settings->setSelectionIncludesAltImageText(true);

#if defined(OS_TIZEN)
  // Scrollbars should not be stylable.
  settings->setAllowCustomScrollbarInMainFrame(false);

  // Don't auto play music on Tizen devices.
  settings->setMediaPlaybackRequiresUserGesture(true);

  // IME support
  settings->setAutoZoomFocusedNodeToLegibleScale(true);

  // Enable double tap to zoom when zoomable.
  settings->setDoubleTapToZoomEnabled(true);
#endif

#if defined(OS_ANDROID)
  settings->setAllowCustomScrollbarInMainFrame(false);
  settings->setTextAutosizingEnabled(prefs.text_autosizing_enabled);
  settings->setTextAutosizingFontScaleFactor(prefs.font_scale_factor);
  web_view->setIgnoreViewportTagScaleLimits(prefs.force_enable_zoom);
  settings->setAutoZoomFocusedNodeToLegibleScale(true);
  settings->setDoubleTapToZoomEnabled(prefs.double_tap_to_zoom_enabled);
  settings->setMediaPlaybackRequiresUserGesture(
      prefs.user_gesture_required_for_media_playback);
  settings->setMediaFullscreenRequiresUserGesture(
      prefs.user_gesture_required_for_media_fullscreen);
  settings->setDefaultVideoPosterURL(
        ASCIIToUTF16(prefs.default_video_poster_url.spec()));
  settings->setSupportDeprecatedTargetDensityDPI(
      prefs.support_deprecated_target_density_dpi);
  settings->setUseLegacyBackgroundSizeShorthandBehavior(
      prefs.use_legacy_background_size_shorthand_behavior);
  settings->setWideViewportQuirkEnabled(prefs.wide_viewport_quirk);
  settings->setUseWideViewport(prefs.use_wide_viewport);
  settings->setViewportMetaLayoutSizeQuirk(
      prefs.viewport_meta_layout_size_quirk);
  settings->setViewportMetaZeroValuesQuirk(
      prefs.viewport_meta_zero_values_quirk);
#endif

  WebNetworkStateNotifier::setOnLine(prefs.is_online);
  settings->setExperimentalWebSocketEnabled(
      prefs.experimental_websocket_enabled);
  settings->setPinchVirtualViewportEnabled(
      prefs.pinch_virtual_viewport_enabled);

  settings->setPinchOverlayScrollbarThickness(
      prefs.pinch_overlay_scrollbar_thickness);
  settings->setUseSolidColorScrollbars(prefs.use_solid_color_scrollbars);
}

}  // namespace content
