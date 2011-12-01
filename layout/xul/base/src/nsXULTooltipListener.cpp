/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#include "nsXULTooltipListener.h"

#include "nsIDOMMouseEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMXULElement.h"
#include "nsIDocument.h"
#include "nsGkAtoms.h"
#include "nsIFrame.h"
#include "nsIPopupBoxObject.h"
#include "nsMenuPopupFrame.h"
#include "nsIServiceManager.h"
#ifdef MOZ_XUL
#include "nsITreeView.h"
#endif
#include "nsGUIEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIScriptContext.h"
#include "nsPIDOMWindow.h"
#ifdef MOZ_XUL
#include "nsXULPopupManager.h"
#endif
#include "nsIRootBox.h"
#include "nsEventDispatcher.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/Element.h"


using namespace mozilla;

nsXULTooltipListener* nsXULTooltipListener::mInstance = nsnull;

//////////////////////////////////////////////////////////////////////////
//// nsISupports

nsXULTooltipListener::nsXULTooltipListener()
  : mMouseScreenX(0)
  , mMouseScreenY(0)
  , mTooltipShownOnce(false)
#ifdef MOZ_XUL
  , mIsSourceTree(false)
  , mNeedTitletip(false)
  , mLastTreeRow(-1)
#endif
{
  if (sTooltipListenerCount++ == 0) {
    // register the callback so we get notified of updates
    Preferences::RegisterCallback(ToolbarTipsPrefChanged,
                                  "browser.chrome.toolbar_tips");

    // Call the pref callback to initialize our state.
    ToolbarTipsPrefChanged("browser.chrome.toolbar_tips", nsnull);
  }
}

nsXULTooltipListener::~nsXULTooltipListener()
{
  if (nsXULTooltipListener::mInstance == this) {
    ClearTooltipCache();
  }
  HideTooltip();

  if (--sTooltipListenerCount == 0) {
    // Unregister our pref observer
    Preferences::UnregisterCallback(ToolbarTipsPrefChanged,
                                    "browser.chrome.toolbar_tips");
  }
}

NS_IMPL_ISUPPORTS1(nsXULTooltipListener, nsIDOMEventListener)

void
nsXULTooltipListener::MouseOut(nsIDOMEvent* aEvent)
{
  // reset flag so that tooltip will display on the next MouseMove
  mTooltipShownOnce = false;

  // if the timer is running and no tooltip is shown, we
  // have to cancel the timer here so that it doesn't 
  // show the tooltip if we move the mouse out of the window
  nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);
  if (mTooltipTimer && !currentTooltip) {
    mTooltipTimer->Cancel();
    mTooltipTimer = nsnull;
    return;
  }

#ifdef DEBUG_crap
  if (mNeedTitletip)
    return;
#endif

#ifdef MOZ_XUL
  // check to see if the mouse left the targetNode, and if so,
  // hide the tooltip
  if (currentTooltip) {
    // which node did the mouse leave?
    nsCOMPtr<nsIDOMEventTarget> eventTarget;
    aEvent->GetTarget(getter_AddRefs(eventTarget));
    nsCOMPtr<nsIDOMNode> targetNode(do_QueryInterface(eventTarget));

    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm) {
      nsCOMPtr<nsIDOMNode> tooltipNode =
        pm->GetLastTriggerTooltipNode(currentTooltip->GetCurrentDoc());
      if (tooltipNode == targetNode) {
        // if the target node is the current tooltip target node, the mouse
        // left the node the tooltip appeared on, so close the tooltip.
        HideTooltip();
        // reset special tree tracking
        if (mIsSourceTree) {
          mLastTreeRow = -1;
          mLastTreeCol = nsnull;
        }
      }
    }
  }
#endif
}

void
nsXULTooltipListener::MouseMove(nsIDOMEvent* aEvent)
{
  if (!sShowTooltips)
    return;

  // stash the coordinates of the event so that we can still get back to it from within the 
  // timer callback. On win32, we'll get a MouseMove event even when a popup goes away --
  // even when the mouse doesn't change position! To get around this, we make sure the
  // mouse has really moved before proceeding.
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(aEvent));
  if (!mouseEvent)
    return;
  PRInt32 newMouseX, newMouseY;
  mouseEvent->GetScreenX(&newMouseX);
  mouseEvent->GetScreenY(&newMouseY);

  // filter out false win32 MouseMove event
  if (mMouseScreenX == newMouseX && mMouseScreenY == newMouseY)
    return;

  // filter out minor movements due to crappy optical mice and shaky hands
  // to prevent tooltips from hiding prematurely.
  nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);

  if ((currentTooltip) &&
      (abs(mMouseScreenX - newMouseX) <= kTooltipMouseMoveTolerance) &&
      (abs(mMouseScreenY - newMouseY) <= kTooltipMouseMoveTolerance))
    return;
  mMouseScreenX = newMouseX;
  mMouseScreenY = newMouseY;

  nsCOMPtr<nsIDOMEventTarget> currentTarget;
  aEvent->GetCurrentTarget(getter_AddRefs(currentTarget));

  nsCOMPtr<nsIContent> sourceContent = do_QueryInterface(currentTarget);
  mSourceNode = do_GetWeakReference(sourceContent);
#ifdef MOZ_XUL
  mIsSourceTree = sourceContent->Tag() == nsGkAtoms::treechildren;
  if (mIsSourceTree)
    CheckTreeBodyMove(mouseEvent);
#endif

  // as the mouse moves, we want to make sure we reset the timer to show it, 
  // so that the delay is from when the mouse stops moving, not when it enters
  // the node.
  KillTooltipTimer();

  // If the mouse moves while the tooltip is up, hide it. If nothing is
  // showing and the tooltip hasn't been displayed since the mouse entered
  // the node, then start the timer to show the tooltip.
  if (!currentTooltip && !mTooltipShownOnce) {
    nsCOMPtr<nsIDOMEventTarget> eventTarget;
    aEvent->GetTarget(getter_AddRefs(eventTarget));

    // don't show tooltips attached to elements outside of a menu popup
    // when hovering over an element inside it. The popupsinherittooltip
    // attribute may be used to disable this behaviour, which is useful for
    // large menu hierarchies such as bookmarks.
    if (!sourceContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::popupsinherittooltip,
                                    nsGkAtoms::_true, eCaseMatters)) {
      nsCOMPtr<nsIContent> targetContent = do_QueryInterface(eventTarget);
      while (targetContent && targetContent != sourceContent) {
        nsIAtom* tag = targetContent->Tag();
        if (targetContent->GetNameSpaceID() == kNameSpaceID_XUL &&
            (tag == nsGkAtoms::menupopup ||
             tag == nsGkAtoms::panel ||
             tag == nsGkAtoms::tooltip)) {
          mSourceNode = nsnull;
          return;
        }

        targetContent = targetContent->GetParent();
      }
    }

    mTooltipTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mTooltipTimer) {
      mTargetNode = do_GetWeakReference(eventTarget);
      if (mTargetNode) {
        nsresult rv = mTooltipTimer->InitWithFuncCallback(sTooltipCallback, this, 
                                                          kTooltipShowTime, nsITimer::TYPE_ONE_SHOT);
        if (NS_FAILED(rv)) {
          mTargetNode = nsnull;
          mSourceNode = nsnull;
        }
      }
    }
    return;
  }

#ifdef MOZ_XUL
  if (mIsSourceTree)
    return;
#endif

  HideTooltip();
  // set a flag so that the tooltip is only displayed once until the mouse
  // leaves the node
  mTooltipShownOnce = true;
}

NS_IMETHODIMP
nsXULTooltipListener::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString type;
  aEvent->GetType(type);
  if (type.EqualsLiteral("DOMMouseScroll") ||
      type.EqualsLiteral("keydown") ||
      type.EqualsLiteral("mousedown") ||
      type.EqualsLiteral("mouseup") ||
      type.EqualsLiteral("dragstart"))
    HideTooltip();
  else if (type.EqualsLiteral("mousemove"))
    MouseMove(aEvent);
  else if (type.EqualsLiteral("mouseout"))
    MouseOut(aEvent);
  else if (type.EqualsLiteral("popuphiding"))
    DestroyTooltip();

  return NS_OK;
}

//////////////////////////////////////////////////////////////////////////
//// nsXULTooltipListener

// static
int
nsXULTooltipListener::ToolbarTipsPrefChanged(const char *aPref,
                                             void *aClosure)
{
  sShowTooltips =
    Preferences::GetBool("browser.chrome.toolbar_tips", sShowTooltips);

  return 0;
}

//////////////////////////////////////////////////////////////////////////
//// nsXULTooltipListener

bool nsXULTooltipListener::sShowTooltips = false;
PRUint32 nsXULTooltipListener::sTooltipListenerCount = 0;

nsresult
nsXULTooltipListener::AddTooltipSupport(nsIContent* aNode)
{
  if (!aNode)
    return NS_ERROR_NULL_POINTER;

  aNode->AddSystemEventListener(NS_LITERAL_STRING("mouseout"), this,
                                false, false);
  aNode->AddSystemEventListener(NS_LITERAL_STRING("mousemove"), this,
                                false, false);
  aNode->AddSystemEventListener(NS_LITERAL_STRING("dragstart"), this,
                                true, false);

  return NS_OK;
}

nsresult
nsXULTooltipListener::RemoveTooltipSupport(nsIContent* aNode)
{
  if (!aNode)
    return NS_ERROR_NULL_POINTER;

  aNode->RemoveSystemEventListener(NS_LITERAL_STRING("mouseout"), this, false);
  aNode->RemoveSystemEventListener(NS_LITERAL_STRING("mousemove"), this, false);
  aNode->RemoveSystemEventListener(NS_LITERAL_STRING("dragstart"), this, true);

  return NS_OK;
}

#ifdef MOZ_XUL
void
nsXULTooltipListener::CheckTreeBodyMove(nsIDOMMouseEvent* aMouseEvent)
{
  nsCOMPtr<nsIContent> sourceNode = do_QueryReferent(mSourceNode);
  if (!sourceNode)
    return;

  // get the boxObject of the documentElement of the document the tree is in
  nsCOMPtr<nsIBoxObject> bx;
  nsIDocument* doc = sourceNode->GetDocument();
  if (doc) {
    nsCOMPtr<nsIDOMElement> docElement = do_QueryInterface(doc->GetRootElement());
    if (docElement) {
      doc->GetBoxObjectFor(docElement, getter_AddRefs(bx));
    }
  }

  nsCOMPtr<nsITreeBoxObject> obx;
  GetSourceTreeBoxObject(getter_AddRefs(obx));
  if (bx && obx) {
    PRInt32 x, y;
    aMouseEvent->GetScreenX(&x);
    aMouseEvent->GetScreenY(&y);

    PRInt32 row;
    nsCOMPtr<nsITreeColumn> col;
    nsCAutoString obj;

    // subtract off the documentElement's boxObject
    PRInt32 boxX, boxY;
    bx->GetScreenX(&boxX);
    bx->GetScreenY(&boxY);
    x -= boxX;
    y -= boxY;

    obx->GetCellAt(x, y, &row, getter_AddRefs(col), obj);

    // determine if we are going to need a titletip
    // XXX check the disabletitletips attribute on the tree content
    mNeedTitletip = false;
    if (row >= 0 && obj.EqualsLiteral("text")) {
      obx->IsCellCropped(row, col, &mNeedTitletip);
    }

    nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);
    if (currentTooltip && (row != mLastTreeRow || col != mLastTreeCol)) {
      HideTooltip();
    } 

    mLastTreeRow = row;
    mLastTreeCol = col;
  }
}
#endif

nsresult
nsXULTooltipListener::ShowTooltip()
{
  nsCOMPtr<nsIContent> sourceNode = do_QueryReferent(mSourceNode);

  // get the tooltip content designated for the target node
  nsCOMPtr<nsIContent> tooltipNode;
  GetTooltipFor(sourceNode, getter_AddRefs(tooltipNode));
  if (!tooltipNode || sourceNode == tooltipNode)
    return NS_ERROR_FAILURE; // the target node doesn't need a tooltip

  // set the node in the document that triggered the tooltip and show it
  nsCOMPtr<nsIDOMXULDocument> xulDoc(do_QueryInterface(tooltipNode->GetDocument()));
  if (xulDoc) {
    // Make sure the target node is still attached to some document. 
    // It might have been deleted.
    if (sourceNode->GetDocument()) {
#ifdef MOZ_XUL
      if (!mIsSourceTree) {
        mLastTreeRow = -1;
        mLastTreeCol = nsnull;
      }
#endif

      mCurrentTooltip = do_GetWeakReference(tooltipNode);
      LaunchTooltip();
      mTargetNode = nsnull;

      nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);
      if (!currentTooltip)
        return NS_OK;

      // listen for popuphidden on the tooltip node, so that we can
      // be sure DestroyPopup is called even if someone else closes the tooltip
      currentTooltip->AddSystemEventListener(NS_LITERAL_STRING("popuphiding"), 
                                             this, false, false);

      // listen for mousedown, mouseup, keydown, and DOMMouseScroll events at document level
      nsIDocument* doc = sourceNode->GetDocument();
      if (doc) {
        // Probably, we should listen to untrusted events for hiding tooltips
        // on content since tooltips might disturb something of web
        // applications.  If we don't specify the aWantsUntrusted of
        // AddSystemEventListener(), the event target sets it to TRUE if the
        // target is in content.
        doc->AddSystemEventListener(NS_LITERAL_STRING("DOMMouseScroll"),
                                    this, true);
        doc->AddSystemEventListener(NS_LITERAL_STRING("mousedown"),
                                    this, true);
        doc->AddSystemEventListener(NS_LITERAL_STRING("mouseup"),
                                    this, true);
        doc->AddSystemEventListener(NS_LITERAL_STRING("keydown"),
                                    this, true);
      }
      mSourceNode = nsnull;
    }
  }

  return NS_OK;
}

#ifdef MOZ_XUL
// XXX: "This stuff inside DEBUG_crap could be used to make tree tooltips work
//       in the future."
#ifdef DEBUG_crap
static void
GetTreeCellCoords(nsITreeBoxObject* aTreeBox, nsIContent* aSourceNode, 
                  PRInt32 aRow, nsITreeColumn* aCol, PRInt32* aX, PRInt32* aY)
{
  PRInt32 junk;
  aTreeBox->GetCoordsForCellItem(aRow, aCol, EmptyCString(), aX, aY, &junk, &junk);
  nsCOMPtr<nsIDOMXULElement> xulEl(do_QueryInterface(aSourceNode));
  nsCOMPtr<nsIBoxObject> bx;
  xulEl->GetBoxObject(getter_AddRefs(bx));
  PRInt32 myX, myY;
  bx->GetX(&myX);
  bx->GetY(&myY);
  *aX += myX;
  *aY += myY;
}
#endif

static void
SetTitletipLabel(nsITreeBoxObject* aTreeBox, nsIContent* aTooltip,
                 PRInt32 aRow, nsITreeColumn* aCol)
{
  nsCOMPtr<nsITreeView> view;
  aTreeBox->GetView(getter_AddRefs(view));
  if (view) {
    nsAutoString label;
#ifdef DEBUG
    nsresult rv = 
#endif
      view->GetCellText(aRow, aCol, label);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Couldn't get the cell text!");
    aTooltip->SetAttr(kNameSpaceID_None, nsGkAtoms::label, label, true);
  }
}
#endif

void
nsXULTooltipListener::LaunchTooltip()
{
  nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);
  if (!currentTooltip)
    return;

#ifdef MOZ_XUL
  if (mIsSourceTree && mNeedTitletip) {
    nsCOMPtr<nsITreeBoxObject> obx;
    GetSourceTreeBoxObject(getter_AddRefs(obx));

    SetTitletipLabel(obx, currentTooltip, mLastTreeRow, mLastTreeCol);
    if (!(currentTooltip = do_QueryReferent(mCurrentTooltip))) {
      // Because of mutation events, currentTooltip can be null.
      return;
    }
    currentTooltip->SetAttr(nsnull, nsGkAtoms::titletip, NS_LITERAL_STRING("true"), true);
  } else {
    currentTooltip->UnsetAttr(nsnull, nsGkAtoms::titletip, true);
  }
  if (!(currentTooltip = do_QueryReferent(mCurrentTooltip))) {
    // Because of mutation events, currentTooltip can be null.
    return;
  }

  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm) {
    nsCOMPtr<nsIContent> target = do_QueryReferent(mTargetNode);
    pm->ShowTooltipAtScreen(currentTooltip, target, mMouseScreenX, mMouseScreenY);

    // Clear the current tooltip if the popup was not opened successfully.
    if (!pm->IsPopupOpen(currentTooltip))
      mCurrentTooltip = nsnull;
  }
#endif

}

nsresult
nsXULTooltipListener::HideTooltip()
{
#ifdef MOZ_XUL
  nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);
  if (currentTooltip) {
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm)
      pm->HidePopup(currentTooltip, false, false, false);
  }
#endif

  DestroyTooltip();
  return NS_OK;
}

static void
GetImmediateChild(nsIContent* aContent, nsIAtom *aTag, nsIContent** aResult) 
{
  *aResult = nsnull;
  PRUint32 childCount = aContent->GetChildCount();
  for (PRUint32 i = 0; i < childCount; i++) {
    nsIContent *child = aContent->GetChildAt(i);

    if (child->Tag() == aTag) {
      *aResult = child;
      NS_ADDREF(*aResult);
      return;
    }
  }

  return;
}

nsresult
nsXULTooltipListener::FindTooltip(nsIContent* aTarget, nsIContent** aTooltip)
{
  if (!aTarget)
    return NS_ERROR_NULL_POINTER;

  // before we go on, make sure that target node still has a window
  nsIDocument *document = aTarget->GetDocument();
  if (!document) {
    NS_WARNING("Unable to retrieve the tooltip node document.");
    return NS_ERROR_FAILURE;
  }
  nsPIDOMWindow *window = document->GetWindow();
  if (!window) {
    return NS_OK;
  }

  bool closed;
  window->GetClosed(&closed);

  if (closed) {
    return NS_OK;
  }

  nsAutoString tooltipText;
  aTarget->GetAttr(kNameSpaceID_None, nsGkAtoms::tooltiptext, tooltipText);
  if (!tooltipText.IsEmpty()) {
    // specifying tooltiptext means we will always use the default tooltip
    nsIRootBox* rootBox = nsIRootBox::GetRootBox(document->GetShell());
    NS_ENSURE_STATE(rootBox);
    *aTooltip = rootBox->GetDefaultTooltip();
    if (*aTooltip) {
      NS_ADDREF(*aTooltip);
      (*aTooltip)->SetAttr(kNameSpaceID_None, nsGkAtoms::label, tooltipText, true);
    }
    return NS_OK;
  }

  nsAutoString tooltipId;
  aTarget->GetAttr(kNameSpaceID_None, nsGkAtoms::tooltip, tooltipId);

  // if tooltip == _child, look for first <tooltip> child
  if (tooltipId.EqualsLiteral("_child")) {
    GetImmediateChild(aTarget, nsGkAtoms::tooltip, aTooltip);
    return NS_OK;
  }

  if (!tooltipId.IsEmpty()) {
    // tooltip must be an id, use getElementById to find it
    nsCOMPtr<nsIContent> tooltipEl = document->GetElementById(tooltipId);

    if (tooltipEl) {
#ifdef MOZ_XUL
      mNeedTitletip = false;
#endif
      tooltipEl.forget(aTooltip);
      return NS_OK;
    }
  }

#ifdef MOZ_XUL
  // titletips should just use the default tooltip
  if (mIsSourceTree && mNeedTitletip) {
    nsIRootBox* rootBox = nsIRootBox::GetRootBox(document->GetShell());
    NS_ENSURE_STATE(rootBox);
    NS_IF_ADDREF(*aTooltip = rootBox->GetDefaultTooltip());
  }
#endif

  return NS_OK;
}


nsresult
nsXULTooltipListener::GetTooltipFor(nsIContent* aTarget, nsIContent** aTooltip)
{
  *aTooltip = nsnull;
  nsCOMPtr<nsIContent> tooltip;
  nsresult rv = FindTooltip(aTarget, getter_AddRefs(tooltip));
  if (NS_FAILED(rv) || !tooltip) {
    return rv;
  }

  // Submenus can't be used as tooltips, see bug 288763.
  nsIContent* parent = tooltip->GetParent();
  if (parent) {
    nsIFrame* frame = parent->GetPrimaryFrame();
    if (frame && frame->GetType() == nsGkAtoms::menuFrame) {
      NS_WARNING("Menu cannot be used as a tooltip");
      return NS_ERROR_FAILURE;
    }
  }

  tooltip.swap(*aTooltip);
  return rv;
}

nsresult
nsXULTooltipListener::DestroyTooltip()
{
  nsCOMPtr<nsIDOMEventListener> kungFuDeathGrip(this);
  nsCOMPtr<nsIContent> currentTooltip = do_QueryReferent(mCurrentTooltip);
  if (currentTooltip) {
    // clear out the tooltip node on the document
    nsCOMPtr<nsIDocument> doc = currentTooltip->GetDocument();
    if (doc) {
      // remove the mousedown and keydown listener from document
      doc->RemoveSystemEventListener(NS_LITERAL_STRING("DOMMouseScroll"), this,
                                     true);
      doc->RemoveSystemEventListener(NS_LITERAL_STRING("mousedown"), this,
                                     true);
      doc->RemoveSystemEventListener(NS_LITERAL_STRING("mouseup"), this, true);
      doc->RemoveSystemEventListener(NS_LITERAL_STRING("keydown"), this, true);
    }

    // remove the popuphidden listener from tooltip
    nsCOMPtr<nsIDOMEventTarget> evtTarget(do_QueryInterface(currentTooltip));

    // release tooltip before removing listener to prevent our destructor from
    // being called recursively (bug 120863)
    mCurrentTooltip = nsnull;

    evtTarget->RemoveEventListener(NS_LITERAL_STRING("popuphiding"), this, false);
  }
  
  // kill any ongoing timers
  KillTooltipTimer();
  mSourceNode = nsnull;
#ifdef MOZ_XUL
  mLastTreeCol = nsnull;
#endif

  return NS_OK;
}

void
nsXULTooltipListener::KillTooltipTimer()
{
  if (mTooltipTimer) {
    mTooltipTimer->Cancel();
    mTooltipTimer = nsnull;
    mTargetNode = nsnull;
  }
}

void
nsXULTooltipListener::sTooltipCallback(nsITimer *aTimer, void *aListener)
{
  nsRefPtr<nsXULTooltipListener> instance = mInstance;
  if (instance)
    instance->ShowTooltip();
}

#ifdef MOZ_XUL
nsresult
nsXULTooltipListener::GetSourceTreeBoxObject(nsITreeBoxObject** aBoxObject)
{
  *aBoxObject = nsnull;

  nsCOMPtr<nsIContent> sourceNode = do_QueryReferent(mSourceNode);
  if (mIsSourceTree && sourceNode) {
    nsCOMPtr<nsIDOMXULElement> xulEl(do_QueryInterface(sourceNode->GetParent()));
    if (xulEl) {
      nsCOMPtr<nsIBoxObject> bx;
      xulEl->GetBoxObject(getter_AddRefs(bx));
      nsCOMPtr<nsITreeBoxObject> obx(do_QueryInterface(bx));
      if (obx) {
        *aBoxObject = obx;
        NS_ADDREF(*aBoxObject);
        return NS_OK;
      }
    }
  }
  return NS_ERROR_FAILURE;
}
#endif
