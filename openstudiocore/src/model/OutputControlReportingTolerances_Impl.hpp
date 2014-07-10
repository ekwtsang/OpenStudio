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

#ifndef MODEL_OUTPUTCONTROLREPORTINGTOLERANCES_IMPL_HPP
#define MODEL_OUTPUTCONTROLREPORTINGTOLERANCES_IMPL_HPP

#include "ModelAPI.hpp"
#include "ModelObject_Impl.hpp"

#include "../utilities/units/Quantity.hpp"
#include "../utilities/units/OSOptionalQuantity.hpp"

namespace openstudio {
namespace model {

namespace detail {

  /** OutputControlReportingTolerances_Impl is a ModelObject_Impl that is the implementation class for OutputControlReportingTolerances.*/
  class MODEL_API OutputControlReportingTolerances_Impl : public ModelObject_Impl {
    Q_OBJECT;

    Q_PROPERTY(double toleranceforTimeHeatingSetpointNotMet READ toleranceforTimeHeatingSetpointNotMet WRITE setToleranceforTimeHeatingSetpointNotMet RESET resetToleranceforTimeHeatingSetpointNotMet);
    Q_PROPERTY(openstudio::Quantity toleranceforTimeHeatingSetpointNotMet_SI READ toleranceforTimeHeatingSetpointNotMet_SI WRITE setToleranceforTimeHeatingSetpointNotMet RESET resetToleranceforTimeHeatingSetpointNotMet);
    Q_PROPERTY(openstudio::Quantity toleranceforTimeHeatingSetpointNotMet_IP READ toleranceforTimeHeatingSetpointNotMet_IP WRITE setToleranceforTimeHeatingSetpointNotMet RESET resetToleranceforTimeHeatingSetpointNotMet);
    Q_PROPERTY(bool isToleranceforTimeHeatingSetpointNotMetDefaulted READ isToleranceforTimeHeatingSetpointNotMetDefaulted);

    Q_PROPERTY(double toleranceforTimeCoolingSetpointNotMet READ toleranceforTimeCoolingSetpointNotMet WRITE setToleranceforTimeCoolingSetpointNotMet RESET resetToleranceforTimeCoolingSetpointNotMet);
    Q_PROPERTY(openstudio::Quantity toleranceforTimeCoolingSetpointNotMet_SI READ toleranceforTimeCoolingSetpointNotMet_SI WRITE setToleranceforTimeCoolingSetpointNotMet RESET resetToleranceforTimeCoolingSetpointNotMet);
    Q_PROPERTY(openstudio::Quantity toleranceforTimeCoolingSetpointNotMet_IP READ toleranceforTimeCoolingSetpointNotMet_IP WRITE setToleranceforTimeCoolingSetpointNotMet RESET resetToleranceforTimeCoolingSetpointNotMet);
    Q_PROPERTY(bool isToleranceforTimeCoolingSetpointNotMetDefaulted READ isToleranceforTimeCoolingSetpointNotMetDefaulted);

    // TODO: Add relationships for objects related to this one, but not pointed to by the underlying data.
    //       Such relationships can be generated by the GenerateRelationships.rb script.
   public:
    /** @name Constructors and Destructors */
    //@{

    OutputControlReportingTolerances_Impl(const IdfObject& idfObject,
                                          Model_Impl* model,
                                          bool keepHandle);

    OutputControlReportingTolerances_Impl(const openstudio::detail::WorkspaceObject_Impl& other,
                                          Model_Impl* model,
                                          bool keepHandle);

    OutputControlReportingTolerances_Impl(const OutputControlReportingTolerances_Impl& other,
                                          Model_Impl* model,
                                          bool keepHandle);

    virtual ~OutputControlReportingTolerances_Impl() {}

    //@}

    /** @name Virtual Methods */
    //@{

    virtual const std::vector<std::string>& outputVariableNames() const;

    virtual IddObjectType iddObjectType() const;

    //@}
    /** @name Getters */
    //@{

    double toleranceforTimeHeatingSetpointNotMet() const;

    Quantity getToleranceforTimeHeatingSetpointNotMet(bool returnIP=false) const;

    bool isToleranceforTimeHeatingSetpointNotMetDefaulted() const;

    double toleranceforTimeCoolingSetpointNotMet() const;

    Quantity getToleranceforTimeCoolingSetpointNotMet(bool returnIP=false) const;

    bool isToleranceforTimeCoolingSetpointNotMetDefaulted() const;

    //@}
    /** @name Setters */
    //@{

    bool setToleranceforTimeHeatingSetpointNotMet(double toleranceforTimeHeatingSetpointNotMet);

    bool setToleranceforTimeHeatingSetpointNotMet(const Quantity& toleranceforTimeHeatingSetpointNotMet);

    void resetToleranceforTimeHeatingSetpointNotMet();

    bool setToleranceforTimeCoolingSetpointNotMet(double toleranceforTimeCoolingSetpointNotMet);

    bool setToleranceforTimeCoolingSetpointNotMet(const Quantity& toleranceforTimeCoolingSetpointNotMet);

    void resetToleranceforTimeCoolingSetpointNotMet();

    //@}
    /** @name Other */
    //@{

    //@}
   protected:
   private:
    REGISTER_LOGGER("openstudio.model.OutputControlReportingTolerances");

    openstudio::Quantity toleranceforTimeHeatingSetpointNotMet_SI() const;
    openstudio::Quantity toleranceforTimeHeatingSetpointNotMet_IP() const;
    openstudio::Quantity toleranceforTimeCoolingSetpointNotMet_SI() const;
    openstudio::Quantity toleranceforTimeCoolingSetpointNotMet_IP() const;
  };

} // detail

} // model
} // openstudio

#endif // MODEL_OUTPUTCONTROLREPORTINGTOLERANCES_IMPL_HPP

