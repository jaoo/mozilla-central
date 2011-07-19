/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=79:
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   David Anderson <dvander@alliedmods.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef jsion_move_resolver_x86_shared_h__
#define jsion_move_resolver_x86_shared_h__

#include "ion/shared/CodeGenerator-shared.h"

namespace js {
namespace ion {

class CodeGenerator;

class MoveResolverX86
{
    bool inCycle_;
    CodeGenerator *codegen;

    RegisterSet freeRegs_;

    // These store stack offsets to spill locations, snapshotting
    // codegen->framePushed_ at the time they were allocated. They are -1 if no
    // stack space has been allocated for that particular spill.
    int32 pushedAtCycle_;
    int32 pushedAtSpill_;
    int32 pushedAtDoubleSpill_;

    // These are registers that are available for temporary use. They may be
    // assigned InvalidReg. If no corresponding spill space has been assigned,
    // then these registers do not need to be spilled.
    Register spilledReg_;
    FloatRegister spilledFloatReg_;

    // These registers are available for resolving cycles.
    Register cycleReg_;
    FloatRegister cycleFloatReg_;

    void assertDone();
    void assertValidMove(const LAllocation *from, const LAllocation *to);
    Register tempReg();
    FloatRegister tempFloatReg();
    Operand cycleSlot() const;
    Operand spillSlot() const;
    Operand doubleSpillSlot() const;

    void emitMove(const LAllocation *from, const LAllocation *to);
    void emitDoubleMove(const LAllocation *from, const LAllocation *to);
    void breakCycle(const LAllocation *from, const LAllocation *to);
    void completeCycle(const LAllocation *from, const LAllocation *to);

  public:
    MoveResolverX86(CodeGenerator *codegen);
    ~MoveResolverX86();
    FloatRegister reserveDouble();
    void setup(LMoveGroup *group);
    void emit(const MoveGroupResolver::Move &move);
    void finish();
};

} // ion
} // js

#endif // jsion_move_resolver_x86_shared_h__

