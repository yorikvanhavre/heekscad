// ObjList.cpp
#include "stdafx.h"
#include "ObjList.h"
#ifdef HEEKSCAD
#include "../src/MarkedList.h"
#else
#include "HeeksCADInterface.h"
#endif
#include "../tinyxml/tinyxml.h"


ObjList::ObjList(const ObjList& objlist): HeeksObj(objlist), m_index_list_valid(true) {operator=(objlist);}

void ObjList::Clear()
{
	std::list<HeeksObj*>::iterator It;
	for(It=m_objects.begin(); It!=m_objects.end() ;It++)
	{
		(*It)->m_owner = NULL;
		delete *It;
	}
	m_objects.clear();
	m_index_list.clear();
	m_index_list_valid = true;
}

const ObjList& ObjList::operator=(const ObjList& objlist)
{
	HeeksObj::operator=(objlist);
	Clear();
	std::list<HeeksObj*>::const_iterator It;
	for (It=objlist.m_objects.begin();It!=objlist.m_objects.end();It++)
	{
		HeeksObj* new_op = (*It)->MakeACopy();
		if(new_op)Add(new_op, NULL);
	}
	return *this;
}

void ObjList::ClearUndoably(void)
{
	if (m_objects.size() == 0) return;
	std::list<HeeksObj*> objects_to_delete = m_objects;
	std::list<HeeksObj*>::iterator It;
	for (It=objects_to_delete.begin();It!=objects_to_delete.end();It++)
	{
#ifdef HEEKSCAD
		wxGetApp().DeleteUndoably(*It);
#else
		heeksCAD->DeleteUndoably(*It);
#endif
	}
	m_objects.clear();
	LoopItStack.clear();
	m_index_list.clear();
	m_index_list_valid = true;
}

HeeksObj* ObjList::MakeACopy(void) const { return new ObjList(*this); }

void ObjList::GetBox(CBox &box)
{
	std::list<HeeksObj*>::iterator It;
	for(It=m_objects.begin(); It!=m_objects.end() ;It++)
	{
		(*It)->GetBox(box);
	}
}

void ObjList::glCommands(bool select, bool marked, bool no_color)
{
	HeeksObj::glCommands(select, marked, no_color);
	std::list<HeeksObj*>::iterator It;
	for(It=m_objects.begin(); It!=m_objects.end() ;It++)
	{
		if(select)glPushName((unsigned int)(*It));
#ifdef HEEKSCAD
		(*It)->glCommands(select, marked || wxGetApp().m_marked_list->ObjectMarked(*It), no_color);
#else
		(*It)->glCommands(select, marked || heeksCAD->ObjectMarked(*It), no_color);
#endif
		if(select)glPopName();
	}
}

HeeksObj* ObjList::GetFirstChild()
{
	if (m_objects.size()==0) return NULL;
	LoopIt = m_objects.begin();
	return *LoopIt;
}

HeeksObj* ObjList::GetNextChild()
{
	if (m_objects.size()==0 || LoopIt==m_objects.end()) return NULL;
	LoopIt++;
	if (LoopIt==m_objects.end()) return NULL;
	return *LoopIt;
}

void ObjList::recalculate_index_list()
{
	m_index_list.clear();
	m_index_list.resize(m_objects.size());
	int i = 0;
	for(std::list<HeeksObj*>::iterator It=m_objects.begin(); It!=m_objects.end() ;It++, i++)
	{
		HeeksObj* object = *It;
		m_index_list[i] = object;
	}
	m_index_list_valid = true;
}

HeeksObj* ObjList::GetAtIndex(int index)
{
	if(!m_index_list_valid)
	{
		recalculate_index_list();
	}

	if(index < 0 || index >= (int)(m_index_list.size()))return NULL;
	return m_index_list[index];
}

int ObjList::GetNumChildren()
{
	return m_objects.size();
}

bool ObjList::Add(HeeksObj* object, HeeksObj* prev_object)
{
	if (object==NULL) return false;
	if (!CanAdd(object)) return false;
	if (m_objects.size()==0 || prev_object==NULL)
	{
		m_objects.push_back(object);
		LoopIt = m_objects.end();
		LoopIt--;
	}
	else
	{
		for(LoopIt = m_objects.begin(); LoopIt != m_objects.end(); LoopIt++) { if (*LoopIt==prev_object) break; }
		m_objects.insert(LoopIt, object);
	}
	m_index_list_valid = false;
	HeeksObj::Add(object, prev_object);

	if(object->GetID() == 0)
	{
#ifdef HEEKSCAD
		object->SetID(wxGetApp().GetNextID(object->GetIDGroupType()));
#else
		object->SetID(heeksCAD->GetNextID(object->GetIDGroupType()));
#endif
	}

	return true;
}


void ObjList::Remove(HeeksObj* object)
{
	if (object==NULL) return;
	for(LoopIt = m_objects.begin(); LoopIt != m_objects.end(); LoopIt++){
		if(*LoopIt==object)break;
	}
	if(LoopIt != m_objects.end())
	{
		m_objects.erase(LoopIt);
	}
	m_index_list_valid = false;
	HeeksObj::Remove(object);

#ifdef HEEKSCAD
	wxGetApp().RemoveID(object);
#else
	heeksCAD->RemoveID(object);
#endif
}

void ObjList::KillGLLists(void)
{
	std::list<HeeksObj*>::iterator It;
	for(It=m_objects.begin(); It!=m_objects.end() ;It++) (*It)->KillGLLists();
}

void ObjList::WriteBaseXML(TiXmlElement *element)
{
	std::list<HeeksObj*>::iterator It;
	for(It=m_objects.begin(); It!=m_objects.end() ;It++) (*It)->WriteXML(element);
}

void ObjList::ReadBaseXML(TiXmlElement* root)
{
	// loop through all the objects
	for(TiXmlElement* pElem = TiXmlHandle(root).FirstChildElement().Element(); pElem;	pElem = pElem->NextSiblingElement())
	{
#ifdef HEEKSCAD
		HeeksObj* object = wxGetApp().ReadXMLElement(pElem);
#else
		HeeksObj* object = heeksCAD->ReadXMLElement(pElem);
#endif
		if(object)Add(object, NULL);
	}
}

void ObjList::ModifyByMatrix(const double *m)
{
	for(std::list<HeeksObj*>::iterator It=m_objects.begin(); It!=m_objects.end() ;It++) (*It)->ModifyByMatrix(m);
}