/*
 * Bookr: document reader for the Sony PSP
 * Copyright (C) 2005 Carlos Carrasco Martinez (carloscm at gmail dot com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <tinyxml2.h>

#include "bkbookmark.h"

/*
<bookmarks>
	<file filename="">
		<lastview ... />
			<viewdata key="" value=""/>
			<viewdata key="" value=""/>
			<viewdata key="" value=""/>
			<viewdata key="" value=""/>
		</lastview>
		<bookmark ... >
			<viewdata key="" value=""/>
			<viewdata key="" value=""/>
			<viewdata key="" value=""/>
			<viewdata key="" value=""/>
			...
		</bookmark>
	</file>
	<lastfile filename=""/>
</bookmarks>

*/
using namespace tinyxml2;

static XMLDocument* doc = 0;
static XMLNode* rootNode = 0;
static XMLElement* root = 0;

static void clearXML() {
	if (doc != 0)
		delete doc;
	doc = new XMLDocument();

	rootNode = doc->NewElement("bookmarks");
	root = rootNode->ToElement();
	root->SetAttribute("version", "2");

	doc->InsertFirstChild(root);
	
	char xmlfilename[1024];
	snprintf(xmlfilename, 1024, BOOKMARK_XML_BASE, FZScreen::basePath().c_str(), BOOKMARK_XML);

	doc->SaveFile(xmlfilename);
}

static void loadXML() {
	char xmlfilename[1024];
	snprintf(xmlfilename, 1024, BOOKMARK_XML_BASE, FZScreen::basePath().c_str(), BOOKMARK_XML);

	if (doc != 0)
		delete doc;

	doc = new XMLDocument();
	doc->LoadFile(xmlfilename);

	if(doc->Error()) {
		// probably file not found, create an empty one
		clearXML();
	}

	// check basic file structure
	if (root == 0) {
		printf("WARNING: corrupted bookmarks file\n");
		clearXML();
	}
	int v = 0;
	root->QueryIntAttribute("version", &v);
	if (v < 2) {
		printf("WARNING: bookmarks file version too old\n");
		clearXML();
	}
}

static void saveXML() {
	char xmlfilename[1024];
	snprintf(xmlfilename, 1024, BOOKMARK_XML_BASE, FZScreen::basePath().c_str(), BOOKMARK_XML);

	if (doc != 0) {
		doc->SaveFile(xmlfilename);
	}
}

static XMLNode* fileNode(string& filename) {
	if (doc == 0)
		loadXML();
	XMLElement* file = root->FirstChildElement("file");
	while (file) {
		const char* name = file->Attribute("filename");
		if (name != 0) {
			if (strncmp(filename.c_str(), name, 1024) == 0)
				return file;
		}
		file = file->NextSiblingElement("file");
	}
	return 0;
}

static XMLElement* lastFileNode() {
	if (doc == 0)
		loadXML();
	XMLElement* file = root->FirstChildElement("lastfile");
	return file;
}

static XMLNode* loadOrAddFileNode(string& filename) {
	if (doc == 0)
		loadXML();
	XMLNode* file = fileNode(filename);
	if (file != 0)
		return file;

	XMLElement *efile = doc->NewElement("file");
	efile->SetAttribute("filename", filename.c_str());
	file = root->InsertEndChild(efile);
	return file;
}

static void loadBookmark(XMLNode* _bn, BKBookmark& b) {
	XMLElement* bn = _bn->ToElement();
	if (strncmp(bn->Value(), "lastview", 1024) == 0) {
		b.lastView = true;
	}
	b.title = bn->Attribute("title");
	int p = 0;
	bn->QueryIntAttribute("page", &p);
	b.page = p;
	b.createdOn = bn->Attribute("createdon");
	//int *thumbnail;
	map<string, int> viewData;
	XMLElement* vd = bn->FirstChildElement("viewdata");
	while (vd) {
		const char* key = vd->Attribute("key");
		int value = 0;
		vd->QueryIntAttribute("value", &value);
		b.viewData.insert(pair<string, int>(key, value));
		vd = vd->NextSiblingElement("viewdata");
	}
}

// find the last read bookmark for a given file
string BKBookmarksManager::getLastFile() {
	XMLElement* file = lastFileNode();
	if (file == 0)
		return string();
	return file->Attribute("filename");
}

// save last use file
void BKBookmarksManager::setLastFile(string& filename) {
	XMLElement* file = lastFileNode();
	if (file != 0)
	{
		file->SetAttribute("filename", filename.c_str());
	} else {
		XMLElement* efile =  doc->NewElement("lastfile");
		efile->SetAttribute("filename", filename.c_str());
		root->InsertEndChild(efile);
	}
	saveXML();
}

// find the last read bookmark for a given file
bool BKBookmarksManager::getLastView(string& filename, BKBookmark& b) {
	XMLNode* file = fileNode(filename);
	if (file == 0)
		return false;
	XMLNode* lastview = file->FirstChildElement("lastview");
	if (lastview == 0)
		return false;
	loadBookmark(lastview, b);
	return true;
}

// add a new bookmark for a file
static void addBookmarkProto(string& filename, BKBookmark& b, XMLNode* file) {
	XMLElement* bookmark = doc->NewElement(b.lastView ? "lastview" : "bookmark");
	bookmark->SetAttribute("title", b.title.c_str());
	bookmark->SetAttribute("page", b.page);
	bookmark->SetAttribute("createdon", b.createdOn.c_str());
	//bookmark->SetAttribute("zoomvalue", b.zoom);
	//bookmark.SetAttribute("thumbnail", );
	map<string, int>::iterator it(b.viewData.begin());
	while (it != b.viewData.end()) {
		XMLElement *vd = doc->NewElement("viewdata");
		vd->SetAttribute("key", (*it).first.c_str());
		vd->SetAttribute("value", (*it).second);
		bookmark->InsertEndChild(vd);
		++it;
	}
	file->InsertEndChild(bookmark);
}

void BKBookmarksManager::addBookmark(string& filename, BKBookmark& b) {
	XMLNode* file = loadOrAddFileNode(filename);
	if (b.lastView) {
		// remove previous lastview
		XMLNode* lastviewnode = file->FirstChildElement("lastview");
		if (lastviewnode != 0)
			file->DeleteChild(lastviewnode);
	}
	addBookmarkProto(filename, b, file);
	saveXML();
}

// load all the bookmarks for a given file
void BKBookmarksManager::getBookmarks(string& filename, BKBookmarkList &bl) {
	XMLNode* file = fileNode(filename);
	if (file == 0)
		return;
	XMLElement* bookmark = file->FirstChildElement();
	while (bookmark != 0) {
		if (strncmp(bookmark->Value(), "bookmark", 1024) == 0) {
			BKBookmark b;
			loadBookmark(bookmark, b);
			bl.push_back(b);
		}
		bookmark = bookmark->NextSiblingElement();
	}
}

// save all the bookmarks for a given file, overwriting the existing ones
void BKBookmarksManager::setBookmarks(string& filename, BKBookmarkList &bl) {
	BKBookmark lastView;
	bool lv = getLastView(filename, lastView);
	XMLNode* file = loadOrAddFileNode(filename);
	file->GetDocument()->Clear();
	if (lv) addBookmarkProto(filename, lastView, file);
	BKBookmarkListIt it(bl.begin());
	while (it != bl.end()) {
		addBookmarkProto(filename, *it, file);
		++it;
	}
	saveXML();
}

void BKBookmarksManager::clear() {
	clearXML();
}

