/*
  Copyright (c) 2004-2010 The FlameRobin Development Team

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


  $Id$

*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <algorithm>
#include <list>

#include "core/Observer.h"
#include "core/Subject.h"
//-----------------------------------------------------------------------------
using namespace std;

typedef list<Observer*>::iterator ObserverIterator;
//-----------------------------------------------------------------------------
Subject::Subject()
{
    locksCountM = 0;
    needsNotifyObjectsM = false;
}
//-----------------------------------------------------------------------------
Subject::~Subject()
{
    detachAllObservers();
}
//-----------------------------------------------------------------------------
void Subject::attachObserver(Observer* observer)
{
    if (observer && observersM.end() == std::find(observersM.begin(),
        observersM.end(), observer))
    {
        observer->addSubject(this);
        observersM.push_back(observer);
        observer->doUpdate();
    }
}
//-----------------------------------------------------------------------------
void Subject::detachObserver(Observer* observer)
{
    if (!observer)
        return;

    observer->removeSubject(this);
    std::list<Observer*>::iterator it = find(observersM.begin(),
        observersM.end(), observer);
    if (it != observersM.end())
        observersM.erase(it);
}
//-----------------------------------------------------------------------------
void Subject::detachAllObservers()
{
    list<Observer*> orig(observersM);
    // make sure there are no reentrancy problems
    // loop over the items in the original list, but ignore all observers
    // which have already been removed from the original list
    for (ObserverIterator it = orig.begin(); it != orig.end(); ++it)
    {
        if (isObservedBy(*it))
            (*it)->removeSubject(this);
    }
    observersM.clear();
}
//-----------------------------------------------------------------------------
bool Subject::isObservedBy(Observer* observer) const
{
    return observersM.end() != std::find(observersM.begin(),
        observersM.end(), observer);
}
//-----------------------------------------------------------------------------
void Subject::notifyObservers()
{
    if (isLocked())
        needsNotifyObjectsM = true;
    else
    {
        list<Observer*> orig(observersM);
        // make sure there are no reentrancy problems
        // loop over the items in the original list, but ignore all observers
        // which have already been removed from the original list
        for (ObserverIterator it = orig.begin(); it != orig.end(); ++it)
        {
            if (isObservedBy(*it))
                (*it)->doUpdate();
        }
        needsNotifyObjectsM = false;
    }
}
//-----------------------------------------------------------------------------
void Subject::lockSubject()
{
    if (!locksCountM)
        lockedChanged(true);
    ++locksCountM;
}
//-----------------------------------------------------------------------------
void Subject::unlockSubject()
{
    if (isLocked())
    {
        --locksCountM;
        if (!isLocked())
        {
            lockedChanged(false);
            if (needsNotifyObjectsM)
                notifyObservers();
        }
    }
}
//-----------------------------------------------------------------------------
void Subject::lockedChanged(bool /*locked*/)
{
}
//-----------------------------------------------------------------------------
unsigned int Subject::getLockCount()
{
    return locksCountM;
}
//-----------------------------------------------------------------------------
bool Subject::isLocked()
{
    return locksCountM > 0;
}
//-----------------------------------------------------------------------------
SubjectLocker::SubjectLocker(Subject* subject)
{
    subjectM = 0;
    setSubject(subject);
}
//-----------------------------------------------------------------------------
SubjectLocker::~SubjectLocker()
{
    setSubject(0);
}
//-----------------------------------------------------------------------------
Subject* SubjectLocker::getSubject()
{
    return subjectM;
}
//-----------------------------------------------------------------------------
void SubjectLocker::setSubject(Subject* subject)
{
    if (subject != subjectM)
    {
        if (subjectM)
            subjectM->unlockSubject();
        subjectM = subject;
        if (subjectM)
            subjectM->lockSubject();
    }
}
//-----------------------------------------------------------------------------
