/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "SplitElementTxn.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIEditorSupport.h"

static NS_DEFINE_IID(kIEditorSupportIID,    NS_IEDITORSUPPORT_IID);

// note that aEditor is not refcounted
SplitElementTxn::SplitElementTxn()
  : EditTxn()
{
}

nsresult SplitElementTxn::Init(nsIEditor  *aEditor,
                               nsIDOMNode *aNode,
                               PRInt32     aOffset)
{
  mEditor = aEditor;
  mExistingRightNode = aNode;
  mOffset = aOffset;
  return NS_OK;
}

SplitElementTxn::~SplitElementTxn()
{
}

nsresult SplitElementTxn::Do(void)
{
  // create a new node
  nsresult result = mExistingRightNode->CloneNode(PR_FALSE, getter_AddRefs(mNewLeftNode));
  NS_ASSERTION(((NS_SUCCEEDED(result)) && (mNewLeftNode)), "could not create element.");

  if ((NS_SUCCEEDED(result)) && (mNewLeftNode))
  {
    // get the parent node
    result = mExistingRightNode->GetParentNode(getter_AddRefs(mParent));
    // insert the new node
    if ((NS_SUCCEEDED(result)) && (mParent))
    {
      nsCOMPtr<nsIEditorSupport> editor;
      result = mEditor->QueryInterface(kIEditorSupportIID, getter_AddRefs(editor));
      if (NS_SUCCEEDED(result) && editor) {
        result = editor->SplitNodeImpl(mExistingRightNode, mOffset, mNewLeftNode, mParent);
      }
      else {
        result = NS_ERROR_NOT_IMPLEMENTED;
      }
    }
  }
  return result;
}

nsresult SplitElementTxn::Undo(void)
{
  // this assumes Do inserted the new node in front of the prior existing node
  nsresult result;
  nsCOMPtr<nsIEditorSupport> editor;
  result = mEditor->QueryInterface(kIEditorSupportIID, getter_AddRefs(editor));
  if (NS_SUCCEEDED(result) && editor) {
    result = editor->JoinNodesImpl(mExistingRightNode, mNewLeftNode, mParent, PR_FALSE);
  }
  else {
    result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

nsresult SplitElementTxn::Redo(void)
{
  nsresult result;
  nsCOMPtr<nsIEditorSupport> editor;
  result = mEditor->QueryInterface(kIEditorSupportIID, getter_AddRefs(editor));
  if (NS_SUCCEEDED(result) && editor) {
    result = editor->SplitNodeImpl(mExistingRightNode, mOffset, mNewLeftNode, mParent);
  }
  else {
    result = NS_ERROR_NOT_IMPLEMENTED;
  }
  return result;
}

nsresult SplitElementTxn::Merge(PRBool *aDidMerge, nsITransaction *aTransaction)
{
  if (nsnull!=aDidMerge)
    *aDidMerge=PR_FALSE;
  return NS_OK;
}

nsresult SplitElementTxn::Write(nsIOutputStream *aOutputStream)
{
  return NS_OK;
}

nsresult SplitElementTxn::GetUndoString(nsString **aString)
{
  if (nsnull!=aString)
  {
    **aString="Join Element";
  }
  return NS_OK;
}

nsresult SplitElementTxn::GetRedoString(nsString **aString)
{
  if (nsnull!=aString)
  {
    **aString="Split Element";
  }
  return NS_OK;
}
