/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkOTTable_OS_2_V0_DEFINED
#define SkOTTable_OS_2_V0_DEFINED

#include "SkEndian.h"
#include "SkIBMFamilyClass.h"
#include "SkOTTableTypes.h"
#include "SkPanose.h"
#include "SkTypedEnum.h"

#pragma pack(push, 1)

struct SkOTTableOS2_V0 {
    SK_OT_USHORT version;
    //The only way to differentiate versionA and version0 is by size.
    static const SK_OT_USHORT version0 = SkTEndian_SwapBE16(0);
    SK_OT_SHORT xAvgCharWidth;
    struct WeightClass {
        SK_TYPED_ENUM(Value, SK_OT_USHORT,
            ((Thin, SkTEndian_SwapBE16(100)))
            ((ExtraLight, SkTEndian_SwapBE16(200)))
            ((Light, SkTEndian_SwapBE16(300)))
            ((Normal, SkTEndian_SwapBE16(400)))
            ((Medium, SkTEndian_SwapBE16(500)))
            ((SemiBold, SkTEndian_SwapBE16(600)))
            ((Bold, SkTEndian_SwapBE16(700)))
            ((ExtraBold, SkTEndian_SwapBE16(800)))
            ((Black, SkTEndian_SwapBE16(900)))
            SK_SEQ_END,
        SK_SEQ_END)
        SK_OT_USHORT value;
    } usWeightClass;
    struct WidthClass {
        SK_TYPED_ENUM(Value, SK_OT_USHORT,
            ((UltraCondensed, SkTEndian_SwapBE16(1)))
            ((ExtraCondensed, SkTEndian_SwapBE16(2)))
            ((Condensed, SkTEndian_SwapBE16(3)))
            ((SemiCondensed, SkTEndian_SwapBE16(4)))
            ((Medium, SkTEndian_SwapBE16(5)))
            ((SemiExpanded, SkTEndian_SwapBE16(6)))
            ((Expanded, SkTEndian_SwapBE16(7)))
            ((ExtraExpanded, SkTEndian_SwapBE16(8)))
            ((UltraExpanded, SkTEndian_SwapBE16(9)))
            SK_SEQ_END,
        (value)SK_SEQ_END)
    } usWidthClass;
    union Type {
        struct Field {
            //8-15
            SK_OT_BYTE_BITFIELD(
                Reserved08,
                Reserved09,
                Reserved10,
                Reserved11,
                Reserved12,
                Reserved13,
                Reserved14,
                Reserved15)
            //0-7
            SK_OT_BYTE_BITFIELD(
                Reserved00,
                Restricted,
                PreviewPrint,
                Editable,
                Reserved04,
                Reserved05,
                Reserved06,
                Reserved07)
        } field;
        struct Raw {
            static const SK_OT_USHORT Installable = SkTEndian_SwapBE16(0);
            static const SK_OT_USHORT RestrictedMask = SkTEndian_SwapBE16(1 << 1);
            static const SK_OT_USHORT PreviewPrintMask = SkTEndian_SwapBE16(1 << 2);
            static const SK_OT_USHORT EditableMask = SkTEndian_SwapBE16(1 << 3);
            SK_OT_USHORT value;
        } raw;
    } fsType;
    SK_OT_SHORT ySubscriptXSize;
    SK_OT_SHORT ySubscriptYSize;
    SK_OT_SHORT ySubscriptXOffset;
    SK_OT_SHORT ySubscriptYOffset;
    SK_OT_SHORT ySuperscriptXSize;
    SK_OT_SHORT ySuperscriptYSize;
    SK_OT_SHORT ySuperscriptXOffset;
    SK_OT_SHORT ySuperscriptYOffset;
    SK_OT_SHORT yStrikeoutSize;
    SK_OT_SHORT yStrikeoutPosition;
    SkIBMFamilyClass sFamilyClass;
    SkPanose panose;
    SK_OT_ULONG ulCharRange[4];
    SK_OT_CHAR achVendID[4];
    union Selection {
        struct Field {
            //8-15
            SK_OT_BYTE_BITFIELD(
                Reserved08,
                Reserved09,
                Reserved10,
                Reserved11,
                Reserved12,
                Reserved13,
                Reserved14,
                Reserved15)
            //0-7
            SK_OT_BYTE_BITFIELD(
                Italic,
                Underscore,
                Negative,
                Outlined,
                Strikeout,
                Bold,
                Regular,
                Reserved07)
        } field;
        struct Raw {
            static const SK_OT_USHORT ItalicMask = SkTEndian_SwapBE16(1 << 0);
            static const SK_OT_USHORT UnderscoreMask = SkTEndian_SwapBE16(1 << 1);
            static const SK_OT_USHORT NegativeMask = SkTEndian_SwapBE16(1 << 2);
            static const SK_OT_USHORT OutlinedMask = SkTEndian_SwapBE16(1 << 3);
            static const SK_OT_USHORT StrikeoutMask = SkTEndian_SwapBE16(1 << 4);
            static const SK_OT_USHORT BoldMask = SkTEndian_SwapBE16(1 << 5);
            static const SK_OT_USHORT RegularMask = SkTEndian_SwapBE16(1 << 6);
            SK_OT_USHORT value;
        } raw;
    } fsSelection;
    SK_OT_USHORT usFirstCharIndex;
    SK_OT_USHORT usLastCharIndex;
    //version0
    SK_OT_SHORT sTypoAscender;
    SK_OT_SHORT sTypoDescender;
    SK_OT_SHORT sTypoLineGap;
    SK_OT_USHORT usWinAscent;
    SK_OT_USHORT usWinDescent;
};

#pragma pack(pop)


SK_COMPILE_ASSERT(sizeof(SkOTTableOS2_V0) == 78, sizeof_SkOTTableOS2_V0_not_78);

#endif
