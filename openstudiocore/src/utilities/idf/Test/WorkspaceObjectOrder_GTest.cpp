/**********************************************************************
*  Copyright (c) 2008-2014, Alliance for Sustainable Energy.  
*  All rights reserved.
*  
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*  
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*  
*  You should have received a copy of the GNU Lesser General Public
*  License along with this library; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**********************************************************************/

#include <gtest/gtest.h>
#include <utilities/idf/Test/IdfFixture.hpp>
#include <utilities/idf/WorkspaceObject.hpp>
#include <utilities/idf/WorkspaceObjectOrder.hpp>
#include <utilities/idd/Material_FieldEnums.hxx>
#include <utilities/idd/Construction_FieldEnums.hxx>

using namespace openstudio;

// Test in context of Workspace, since order needs objectGetter.
TEST_F(IdfFixture,WorkspaceObjectOrder) {
  Workspace workspace(IdfFixture::epIdfFile,openstudio::StrictnessLevel::Draft);

  WorkspaceObjectOrder wsOrder = workspace.order();

  EXPECT_TRUE(wsOrder.inOrder(workspace.objects()[0].handle()));

  // save current order
  OptionalHandleVector workspaceOrder = wsOrder.directOrder();
  ASSERT_TRUE(workspaceOrder);

  // order by enum
  wsOrder.setOrderByIddEnum();
  WorkspaceObjectVector objects = workspace.objects(true);
  for (WorkspaceObjectVector::const_iterator it = objects.begin(), itEnd = objects.end() - 1;
       it != itEnd; ++ it) {
    WorkspaceObjectVector::const_iterator nxt = it; ++nxt;
    EXPECT_TRUE(it->iddObject().type() <= nxt->iddObject().type());
  }

  // restore order
  wsOrder.setDirectOrder(*workspaceOrder);
  HandleVector handles = workspace.handles(true);
  // handles does not include version object, while direct order does
  HandleVector tempOrder = *workspaceOrder;
  HandleVector::iterator it = std::find(tempOrder.begin(),tempOrder.end(),workspace.versionObject()->handle());
  tempOrder.erase(it);
  EXPECT_TRUE(tempOrder == handles);

  // move objects directly
  wsOrder.insert(handles[32],handles[12]);
  HandleVector newOrder = workspace.handles(true);
  EXPECT_EQ(handles.size(),newOrder.size());
  EXPECT_TRUE(handles[32] == newOrder[12]);
  EXPECT_TRUE(handles[12] == newOrder[13]);

  wsOrder.swap(handles[80],handles[100]);
  newOrder = workspace.handles(true);
  EXPECT_EQ(handles.size(),newOrder.size());
  EXPECT_TRUE(handles[80] == newOrder[100]);
  EXPECT_TRUE(handles[100] == newOrder[80]);
}

TEST_F(IdfFixture,WorkspaceObjectOrder_ByIddObjectType) {
  Workspace workspace(IdfFixture::epIdfFile,openstudio::StrictnessLevel::Draft);
  WorkspaceObjectVector objectsInOriginalOrder = workspace.objects(true);

  WorkspaceObjectOrder wsOrder = workspace.order();
  IddObjectTypeVector orderByType;
  StringSet enumValues = IddObjectType::getStringValues();
  BOOST_FOREACH(std::string val,enumValues) {
    orderByType.push_back(IddObjectType(val));
  }
  wsOrder.setIddOrder(orderByType);
  WorkspaceObjectVector objectsInNewOrder = workspace.objects(true);
  EXPECT_EQ(objectsInOriginalOrder.size(),objectsInNewOrder.size());
  EXPECT_FALSE(objectsInOriginalOrder == objectsInNewOrder);

  // expect Materials before Constructions
  OptionalWorkspaceObject oMaterial;
  OptionalWorkspaceObject oConstruction;
  BOOST_FOREACH(const WorkspaceObject& object,objectsInNewOrder) {
    if (!oMaterial && (object.iddObject().type() == iddobjectname::Material)) {
      oMaterial = object;
      EXPECT_FALSE(oConstruction);
      OptionalUnsigned oIndex = wsOrder.indexInOrder(object.handle());
      EXPECT_FALSE(oIndex);
    }
    if (!oConstruction && (object.iddObject().type() == iddobjectname::Construction)) {
      oConstruction = object;
    }
    if (oMaterial && oConstruction) { break; }
  }

  // change to Constructions before Materials
  wsOrder.move(iddobjectname::Construction,iddobjectname::Material);
  objectsInNewOrder = workspace.objects(true);
  oMaterial = boost::none;
  oConstruction = boost::none;
  BOOST_FOREACH(const WorkspaceObject& object,objectsInNewOrder) {
    if (!oMaterial && (object.iddObject().type() == iddobjectname::Material)) {
      oMaterial = object;
    }
    if (!oConstruction && (object.iddObject().type() == iddobjectname::Construction)) {
      oConstruction = object;
      EXPECT_FALSE(oMaterial);
    }
    if (oMaterial && oConstruction) { break; }
  }

}
