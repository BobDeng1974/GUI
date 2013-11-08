/*
	This file is part of the GUI library.
	Copyright (C) 2008-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2008-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2008-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TreeView.h"
#include "../GUI_Manager.h"
#include "../Base/AbstractShape.h"
#include "../Base/AnimationHandler.h"
#include "../Base/Layouters/ExtLayouter.h"
#include "ComponentPropertyIds.h"
#include "Scrollbar.h"
#include <Util/UI/Event.h>
#include <functional>
#include <iostream>

namespace GUI {

static const Util::StringIdentifier dataId_scrollPos("scroll");

//! TV_ScrollAnimation ---|> AnimationHandler
class TV_ScrollAnimation:public AnimationHandler{
	public:
		float targetPos;

		TV_ScrollAnimation(TreeView * tv,float _targetPos, float _duration) :
			AnimationHandler(tv,_duration),targetPos(_targetPos){
		}
		virtual ~TV_ScrollAnimation()	{}

		// ---|> AnimationHandler
		bool animate(float t) override{
			if(t > getEndTime()) {
				finish();
				return false;
			}
			TreeView * tv= dynamic_cast<TreeView *>(getComponent());
			tv->scrollTo( (tv->getScrollPos()*2 + targetPos ) /3.0 );
			return true;
		}

		// ---|> AnimationHandler
		void finish() override{
			dynamic_cast<TreeView *>(getComponent())->scrollTo(targetPos);
		}
};
// ------------------------------------------------------------------------------------------------------------

//! (TreeView::TreeViewEntry] [ctor)
TreeView::TreeViewEntry::TreeViewEntry(GUI_Manager & _gui,TreeView * _treeView,Component * c/*=nullptr*/,flag_t _flags/*=0*/):
		Container(_gui,_flags),MouseButtonListener(),
		myTreeView(_treeView),marked(false) {
	if(c) {
		Container::_insertAfter(c,getLastChild());
	}
	addMouseButtonListener(this);
	//ctor
}

//! (TreeView::TreeViewEntry] [dtor)
TreeView::TreeViewEntry::~TreeViewEntry() {
	myTreeView = nullptr;
	//ctor
}

//! [TreeView::TreeViewEntry] ---|> Component
void TreeView::TreeViewEntry::doDisplay(const Geometry::Rect & region) {
	if(myTreeView==nullptr ||
			getAbsPosition().getY()+getHeight()<myTreeView->getAbsPosition().getY() ||
			getAbsPosition().getY()>myTreeView->getAbsPosition().getY()+myTreeView->getHeight() )
		return;

	if(isSelected())
		getGUI().displayShape(PROPERTY_TREEVIEW_ENTRY_SELECTION_SHAPE,getLocalRect());

	if(isMarked() && getFirstChild()!=nullptr) {
		Geometry::Rect r=getFirstChild()->getLocalRect();
		r.setWidth(myTreeView->getWidth());
		getGUI().displayShape(PROPERTY_TREEVIEW_ENTRY_MARKING_SHAPE,r);
	}

	for (Component * c=getFirstChild();c!=nullptr;c=c->getNext()) {
		if(c->isEnabled()) {
			c->display(region);
			// if item is collapsed show only first element
			if(isCollapsed())
				break;
		}
	}
	if(getContentsCount() > 1 && dynamic_cast<TreeViewEntry*>(getParent())) {
		float h=getFirstChild()->getHeight();
		if(isCollapsed())
			getGUI().displayShape(PROPERTY_TREEVIEW_ACTIVE_INDENTATION_SHAPE,Geometry::Rect(4,0,1,getHeight()));
		else
			getGUI().displayShape(PROPERTY_TREEVIEW_PASSIVE_INDENTATION_SHAPE,Geometry::Rect(4,0,1,getHeight()));


		getGUI().displayShape(PROPERTY_TREEVIEW_SUBROUP_SHAPE,Geometry::Rect(1,1,8,h-2),isCollapsed() ? 0 : AbstractShape::ACTIVE);
	}else{
		getGUI().displayShape(PROPERTY_TREEVIEW_ACTIVE_INDENTATION_SHAPE,Geometry::Rect(4,0,1,getHeight()));
	}

}

//! ---|> MouseButtonListener
listenerResult_t TreeView::TreeViewEntry::onMouseButton(Component * /*component*/, const Util::UI::ButtonEvent & buttonEvent){
	if(myTreeView==nullptr)
		return LISTENER_EVENT_NOT_CONSUMED;
	if(!buttonEvent.pressed)
		return LISTENER_EVENT_NOT_CONSUMED;
	if(buttonEvent.button == Util::UI::MOUSE_BUTTON_LEFT) {
		const Geometry::Vec2 localPos = Geometry::Vec2(buttonEvent.x, buttonEvent.y) - getAbsPosition();
		if(localPos.getX()<10&&localPos.getY()<20&& getContentsCount()>1) {
			if(isCollapsed())
				open();
			else
				collapse();
		} else if(getFirstChild() && localPos.getY()<getFirstChild()->getHeight()) {
			activate();
			select();
			if(getGUI().isCtrlPressed()) {
				if(isMarked())
					myTreeView->unmarkEntry(this);
				else
					myTreeView->markEntry(this);
			} else {
				myTreeView->unmarkAll();
				myTreeView->markEntry(this);
			}
			myTreeView->markingChanged();
		}
		return LISTENER_EVENT_CONSUMED;
	} else if(buttonEvent.button == Util::UI::MOUSE_BUTTON_RIGHT) {
		myTreeView->unmarkAll();
		myTreeView->markingChanged();
		return LISTENER_EVENT_CONSUMED;
	}
	return LISTENER_EVENT_NOT_CONSUMED;
}

const Util::StringIdentifier TreeView::TreeViewEntry::ACTION_TreeViewEntry_collapse("TreeViewEntry_collapse");
const Util::StringIdentifier TreeView::TreeViewEntry::ACTION_TreeViewEntry_open("TreeViewEntry_open");

void TreeView::TreeViewEntry::collapse(){
	getGUI().componentActionPerformed(this,ACTION_TreeViewEntry_collapse);
	setFlag(COLLAPSED_ENTRY, true);
	invalidateLayout();
	if(hasParent())
		getParent()->invalidateLayout();
	if(myTreeView){
		myTreeView->invalidateLayout();
		myTreeView->invalidateRegion();
	}
}
void TreeView::TreeViewEntry::open(){
	getGUI().componentActionPerformed(this,ACTION_TreeViewEntry_open);
	setFlag(COLLAPSED_ENTRY, false);
	invalidateLayout();
	if(hasParent())
		getParent()->invalidateLayout();
	if(myTreeView){
		myTreeView->invalidateLayout();
		myTreeView->invalidateRegion();
	}
}

//! [TreeView::TreeViewEntry] ---|> Container
void TreeView::TreeViewEntry::insertAfter(const Ref & child,const Ref & after) {
	if(child.isNull()) {
		return;
	} else if(!getFirstChild() && !(myTreeView && myTreeView->getRootEntry()==this) ){ // if the entry is empty and is not the root node, use the child as entry
		if(myTreeView!=nullptr)
			myTreeView->invalidateRegion();
		_insertAfter(child,after);
	}else{
		TreeViewEntry * e=dynamic_cast<TreeViewEntry*>(child.get());
		if(e==nullptr) {
			e=new TreeViewEntry(getGUI(),myTreeView,child.get());
		} else {
			e->setTreeView(myTreeView);
		}
		if(myTreeView!=nullptr)
			myTreeView->invalidateRegion();
		_insertAfter(e,after);
	}
}
//
//! [TreeView::TreeViewEntry] ---|> Container
void TreeView::TreeViewEntry::addContent(const Ref & child) {
	insertAfter(child,getLastChild());
}

//! [TreeView::TreeViewEntry] ---|> Container
void TreeView::TreeViewEntry::insertBefore(const Ref & child,const Ref & after) {
	if(child.isNull()) return;

//	std::cout << " <addEntry2 ";
	TreeViewEntry * e=dynamic_cast<TreeViewEntry*>(child.get());
	if(e==nullptr) {
		e=new TreeViewEntry(getGUI(),myTreeView,child.get());
	} else {
		e->setTreeView(myTreeView);
	}
	if(myTreeView!=nullptr)
		myTreeView->invalidateRegion();
	Container::insertBefore(e,after);
}

//! [TreeView::TreeViewEntry] ---|> Container
void TreeView::TreeViewEntry::removeContent(const Ref & child) {
	if(child.isNull())
		return;

	unmarkSubtree(child.get());
	TreeViewEntry * e = dynamic_cast<TreeViewEntry*>(child.get());

	if(e==nullptr)
		e = dynamic_cast<TreeViewEntry*>(child->getParent());

	if(e==nullptr){
		std::cerr << "Wrong parent!";
		return;
	}
	Container::removeContent(e);
}

//! [TreeView::TreeViewEntry] ---|> Container
void TreeView::TreeViewEntry::clearContents() {
	unmarkSubtree(this);
	Container::clearContents();
}

//! [TreeView::TreeViewEntry] ---|> Container
std::vector<Component*> TreeView::TreeViewEntry::getContents(){
	std::vector<Component*> children;
	for(Component * c=getFirstChild();c!=nullptr;c=c->getNext()){
			children.push_back(c);
	}
	return children;
}

//! (TreeView::TreeViewEntry)
TreeView::TreeViewEntry * TreeView::TreeViewEntry::getFirstSubentry()const{
	for (Component * c=getFirstChild();c!=nullptr;c=c->getNext()) {
		TreeViewEntry * e=dynamic_cast<TreeViewEntry*>(c);
		if(e)
			return e;
	}
	return nullptr;
}

//! (TreeView::TreeViewEntry)
void TreeView::TreeViewEntry::setComponent(const Ref & c){
	Component * first = getFirstChild();
	if(c!=first){
		if(dynamic_cast<TreeViewEntry*>(first)==nullptr) // there is an old component? -> remove it
			Container::removeContent(first);
		Container::insertBefore(c,getFirstChild());
		if(myTreeView!=nullptr)
			myTreeView->invalidateRegion();
	}
}

//! (TreeView::TreeViewEntry) ---|> Component
void TreeView::TreeViewEntry::doLayout() {
	if(myTreeView==nullptr)
		return;
	uint32_t h=0;

	// \note Even if isCollapsed(), the other entries need to be moved out of the way 
	//   so that they do not grab the click events for the first component.
	for (Component * c=getFirstChild();c!=nullptr;c=c->getNext()) {
		c->setPosition(Geometry::Vec2(10,h));
		h += static_cast<uint32_t>(c->getHeight());
	}
	setSize( getParent()->getWidth()-10,
			(isCollapsed()&&getFirstChild()!=nullptr) ? 
				getFirstChild()->getHeight() : h);
}

//! (TreeView::TreeViewEntry)
void TreeView::TreeViewEntry::setTreeView( TreeView * tv) {
	myTreeView=tv;

	for (Component * c=getFirstChild();c!=nullptr;c=c->getNext()) {
		TreeViewEntry * e=dynamic_cast<TreeViewEntry*>(c);
		if(e) {
			e->setTreeView(myTreeView);
		}
	}
}

//! (TreeView::TreeViewEntry) ( internal)
void TreeView::TreeViewEntry::unmarkSubtree(Component * subroot)const{
	if(getTreeView()==nullptr) // no treeView set -> no marking possible
		return;

	struct MyVisitor : public Visitor {
		TreeView * treeView;
		MyVisitor(TreeView * tw) : treeView(tw){}
		virtual ~MyVisitor() {};

		visitorResult_t visit(Component & c) override{
			TreeViewEntry * e = dynamic_cast<TreeViewEntry *>(&c);
			if(e!=nullptr && e->isMarked() && e->getTreeView()==treeView){
				treeView->unmarkEntry(e);
			}
			return CONTINUE_TRAVERSAL;
		};
	}visitor(getTreeView());
	subroot->traverseSubtree(visitor);
}

//// ------------------------------------------------------------------------------

//! (ctor)
TreeView::TreeView(GUI_Manager & _gui,const std::string & _actionName,flag_t _flags/*=0*/):
		Container(_gui,_flags),DataChangeListener(),MouseButtonListener(),MouseMotionListener(),KeyListener(),
		actionName(_actionName),root(new TreeViewEntry(_gui,this)),scrollPos(0),multiSelect(true),scrollBar(nullptr) {
	init();
	//ctor
}

//! (ctor)
TreeView::TreeView(GUI_Manager & _gui,const Geometry::Rect & _r,const std::string & _actionName,flag_t _flags/*=0*/):
		Container(_gui,_r,_flags),DataChangeListener(),MouseButtonListener(),MouseMotionListener(),KeyListener(),
		actionName(_actionName),root(new TreeViewEntry(_gui,this)),scrollPos(0),multiSelect(true),scrollBar(nullptr) {
	init();
	//ctor
}

//! (dtor)
TreeView::~TreeView() {
	// destroy root
	getGUI().removeMouseMotionListener(this);
	root=nullptr;
	//dtor
}

void TreeView::init() {
	setFlag(SELECTABLE,true);

	_addChild(root.get());
	addMouseButtonListener(this);
	addKeyListener(this);

	setFlag(USE_SCISSOR,true);
	setFlag(LOWERED_BORDER,true);
//	setFlag(BACKGROUND,true);
}

//! ---|> Component
void TreeView::doLayout() {
	if(root->getHeight()>=getHeight()){
		if(scrollBar.isNull()){
			scrollBar = new Scrollbar(getGUI(),dataId_scrollPos,Scrollbar::VERTICAL);
			scrollBar->setExtLayout( 	ExtLayouter::POS_X_ABS|ExtLayouter::REFERENCE_X_RIGHT|ExtLayouter::ALIGN_X_RIGHT|
											ExtLayouter::POS_Y_ABS|ExtLayouter::REFERENCE_Y_TOP|ExtLayouter::ALIGN_Y_TOP |
											ExtLayouter::WIDTH_ABS|ExtLayouter::HEIGHT_ABS,
											Geometry::Vec2(1,2),Geometry::Vec2(getGUI().getGlobalValue(PROPERTY_SCROLLBAR_WIDTH),-4));
			scrollBar->addDataChangeListener(this);
			_addChild(scrollBar.get());
		}
		const int maxScrollPos = std::max(0,static_cast<int>(root->getHeight()-getHeight()));
		scrollPos = std::min(root->getHeight()-getHeight(),scrollPos);
		scrollBar->setMaxScrollPos(maxScrollPos); 
		scrollBar->setScrollPos( scrollPos );
	}else{
		scrollPos = 0;
		if(scrollBar.isNotNull()){
			Component::destroy(scrollBar.get());
			scrollBar = nullptr;
		}
	}
	root->setPosition(Geometry::Vec2(-10,-scrollPos));

}

//! ---|> Component
void TreeView::doDisplay(const Geometry::Rect & region) {

	if(isSelected()) {
		Geometry::Rect r = getLocalRect();
		r.changeSizeCentered(-2, -2);
		getGUI().displayShape(PROPERTY_SELECTION_RECT_SHAPE,r);
	}
	displayChildren(region,true);
	if(scrollBar.isNotNull()){
		if(scrollPos>0)
			getGUI().displayShape(PROPERTY_SCROLLABLE_MARKER_TOP_SHAPE, getLocalRect(), 0);
		if(scrollPos<scrollBar->getMaxScrollPos())
			getGUI().displayShape(PROPERTY_SCROLLABLE_MARKER_BOTTOM_SHAPE, getLocalRect(), 0);
	}
}

/**
 * ---|> KeyListener
 * TODO: Handle collapsed entries, allow collapsing of entries, scroll to marked entry, allow multiple selection with keyboard...
 */
bool TreeView::onKeyEvent(Component * /*component*/, const Util::UI::KeyboardEvent & keyEvent) {
	if(!keyEvent.pressed)
		return true;
	else if(keyEvent.key == Util::UI::KEY_TAB){
		return false;
	}
	else if(keyEvent.key == Util::UI::KEY_UP) {
//		scroll(-15);
		if(markedEntries.size()==1){
			TreeViewEntry * m = markedEntries.front().get();
			TreeViewEntry * newEntry=dynamic_cast<TreeViewEntry*>(m->getPrev());

			if(newEntry){
				if(dynamic_cast<TreeViewEntry*>(newEntry->getLastChild())){
					newEntry=dynamic_cast<TreeViewEntry*>(newEntry->getLastChild());
				}
			}else{
				if(dynamic_cast<TreeViewEntry*>(m->getParent())){
					newEntry=dynamic_cast<TreeViewEntry*>(m->getParent());
				}
			}
			if(newEntry){
				unmarkEntry(m);
				markEntry(newEntry);
				markingChanged();
			}
		}

	}
	else if(keyEvent.key == Util::UI::KEY_DOWN) {
//		scroll(15);
		if(markedEntries.size()==1){
			TreeViewEntry * newEntry = nullptr;
			TreeViewEntry * m = markedEntries.front().get();
			if(m->getContentsCount()>1 && dynamic_cast<TreeViewEntry*>(m->getFirstChild()->getNext())){
				newEntry=dynamic_cast<TreeViewEntry*>(m->getFirstChild()->getNext());
			}else{
				newEntry=dynamic_cast<TreeViewEntry*>(m->getNext());
				if(!newEntry){
					if(dynamic_cast<TreeViewEntry*>(m->getParent())){
						newEntry=dynamic_cast<TreeViewEntry*>(m->getParent()->getNext());
					}
				}
			}
			if(newEntry){
				unmarkEntry(m);
				markEntry(newEntry);
				markingChanged();
			}
		}
	}
	return true;
}

//! ---|> Container
void TreeView::addContent(const Ref & child) {
	if(child.isNull()) return;
	invalidateRegion();
	root->addContent(child);
}

//! ---|> Container
void TreeView::removeContent(const Ref & child) {
	if(child.isNull()) return;
	invalidateRegion();
	root->removeContent(child);
}

//! ---|> Container
void TreeView::clearContents() {
	unmarkAll();
	root->clearContents();
	scrollPos = 0;
}

//! ---|> MouseButtonListener
listenerResult_t TreeView::onMouseButton(Component * /*component*/, const Util::UI::ButtonEvent & buttonEvent){
	if(buttonEvent.button == Util::UI::MOUSE_BUTTON_MIDDLE && scrollBar.isNotNull()) {
		if(buttonEvent.pressed) {
			getGUI().addMouseMotionListener(this);
		} else {// !pressed
			getGUI().removeMouseMotionListener(this);
		}
		return LISTENER_EVENT_CONSUMED;
	} else if(buttonEvent.pressed && buttonEvent.button == Util::UI::MOUSE_WHEEL_UP) {
//		scroll(-getHeight()*0.25);

		getGUI().stopAnimations(this);
		getGUI().addAnimationHandler(new TV_ScrollAnimation(this, scrollPos-getHeight()*0.5 ,0.3));

		return LISTENER_EVENT_CONSUMED;
	}else if(buttonEvent.pressed && buttonEvent.button == Util::UI::MOUSE_WHEEL_DOWN) {
//		scroll(+getHeight()*0.25);
		getGUI().stopAnimations(this);
		getGUI().addAnimationHandler(new TV_ScrollAnimation(this, scrollPos+getHeight()*0.5 ,0.3));
		return LISTENER_EVENT_CONSUMED;
	}else{
		return LISTENER_EVENT_NOT_CONSUMED;
	}
}

//! ---|> MouseMotionListener
listenerResult_t TreeView::onMouseMove(Component * /*component*/, const Util::UI::MotionEvent & motionEvent) {
	if(!(motionEvent.buttonMask & Util::UI::MASK_MOUSE_BUTTON_MIDDLE)) {
		return LISTENER_EVENT_NOT_CONSUMED_AND_REMOVE_LISTENER;
	}
	scroll(motionEvent.deltaY * -2.0);
	return LISTENER_EVENT_CONSUMED;
}
void TreeView::scroll(float amount) {
	scrollTo(scrollPos+amount);
}


void TreeView::scrollTo(float _position) {
	const float oldScrollPos=scrollPos;
	scrollPos=_position;
	if(scrollPos<0 || root->getHeight()<getHeight() ){
		
		scrollPos=0;
	}else if(scrollPos>root->getHeight() - getHeight()){
		scrollPos=root->getHeight() - getHeight();
	}
	if(scrollPos!=oldScrollPos){
		invalidateLayout();
		invalidateRegion();
	}
}

void TreeView::scrollToSelection() {
	if(!markedEntries.empty()) {
		TreeViewEntry * entry = markedEntries.front().get();
		const float yPos = entry->getAbsPosition().y()-getAbsPosition().y()+scrollPos;


		if( yPos < scrollPos+entry->getHeight()*0.5 ){
			getGUI().stopAnimations(this);
			getGUI().addAnimationHandler(new TV_ScrollAnimation(this,yPos - entry->getHeight()*0.5 ,0.4));
		}
		else if( yPos+entry->getHeight()*2.0 > scrollPos+getHeight() ){
			getGUI().stopAnimations(this);
			getGUI().addAnimationHandler(new TV_ScrollAnimation(this,yPos - std::max(getHeight()-entry->getHeight()*3.0,getHeight()*0.3 ),0.4));
		}
	}
//	std::cout << getHeight();
}


void TreeView::markEntry(TreeViewEntry * entry) {
	if(entry && entry->getTreeView()==this && !entry->isMarked()){
		if(!multiSelect)
			unmarkAll();
		markedEntries.push_back(entry);
		entry->_setMarked(true);

		if(markedEntries.size()==1)
			scrollToSelection();
	}
}


void TreeView::unmarkEntry(TreeViewEntry * entry) {
	if(entry && entry->isMarked()){
		const auto pos = std::find(markedEntries.begin(),markedEntries.end(),entry);
		if(pos!=markedEntries.end()){
			markedEntries.erase(pos);
			entry->_setMarked(false);
		}
	}
}


void TreeView::unmarkAll() {
	for(auto entry : markedEntries)
		entry->_setMarked(false);
	markedEntries.clear();
}


void TreeView::markComponent(Component * c){
	if(c!=nullptr) 
		markEntry(dynamic_cast<TreeViewEntry *>(c->getParent()));
}


void TreeView::unmarkComponent(Component * c){
	if(c!=nullptr)
		unmarkEntry(dynamic_cast<TreeViewEntry *>(c->getParent()));
}


std::vector<Component*> TreeView::getMarkedComponents(){
	std::vector<Component*> arr;
	for(auto & entry : markedEntries) {
		Component * c = entry->getFirstChild();
		if( dynamic_cast<TreeViewEntry *>(c) == nullptr ) // should be the real content, not an entry...
			arr.push_back(c);
	}
	return arr;
}


void TreeView::markingChanged(){
	getGUI().componentDataChanged(this,actionName);
}

//! ---|> DataChangeListener
void TreeView::handleDataChange(Component *,const Util::StringIdentifier & _actionName){
	if(_actionName==dataId_scrollPos && scrollBar.isNotNull()){
		invalidateRegion();
		invalidateLayout();
		scrollTo(scrollBar->getScrollPos());
	}
}

//! ---|> Container
std::vector<Component*> TreeView::getContents(){
	return root->getContents();
}

}