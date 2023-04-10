/**
 * @file GeneralFunctionDialog.h
 * Declares the GeneralFunctionDialog class.
 * @ingroup meshtex-ui
 */

/*
 * Copyright 2012 Joel Baxter
 *
 * This file is part of MeshTex.
 *
 * MeshTex is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * MeshTex is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MeshTex.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "GenericDialog.h"
#include "MeshVisitor.h"

/**
 * Subclass of GenericDialog that implements the window summoned by selecting
 * the General Function menu entry. This window is used to specify equations
 * for setting S and T values as a linear combination of various factors.
 *
 * @image html genfunc.png
 *
 * @ingroup meshtex-ui
 */
class GeneralFunctionDialog : public GenericDialog
{
private: // private types

   /**
    * Visitor for applying the chosen equation to a mesh.
    */
   class GeneralFunctionVisitor : public MeshVisitor
   {
   public:
      GeneralFunctionVisitor(const MeshEntity::GeneralFunctionFactors *sFactors,
                             const MeshEntity::GeneralFunctionFactors *tFactors,
                             const MeshEntity::SliceDesignation *alignRow,
                             const MeshEntity::SliceDesignation *alignCol,
                             const MeshEntity::RefSliceDescriptor *refRow,
                             const MeshEntity::RefSliceDescriptor *refCol,
                             bool surfaceValues);
   private:
      bool Execute(MeshEntity& meshEntity) const;
   private:
      const MeshEntity::GeneralFunctionFactors *_sFactors;
      const MeshEntity::GeneralFunctionFactors *_tFactors;
      const MeshEntity::SliceDesignation *_alignRow;
      const MeshEntity::SliceDesignation *_alignCol;
      const MeshEntity::RefSliceDescriptor *_refRow;
      const MeshEntity::RefSliceDescriptor *_refCol;
      bool _surfaceValues;
   };

public: // public methods

   GeneralFunctionDialog(const std::string& key);
   ~GeneralFunctionDialog();
   bool Apply();

private: // private member vars

   QRadioButton *surface;

   QCheckBox *s_apply;
   QDoubleSpinBox *s_oldval, *s_rowdist, *s_coldist, *s_rownum, *s_colnum, *s_constant;
   QCheckBox *t_apply;
   QDoubleSpinBox *t_oldval, *t_rowdist, *t_coldist, *t_rownum, *t_colnum, *t_constant;

   QSpinBox *col_num_align;
   QGroupBox *row_ref;
   QSpinBox *row_num_ref;
   QCheckBox *row_ref_total;

   QSpinBox *row_num_align;
   QGroupBox *col_ref;
   QSpinBox *col_num_ref;
   QCheckBox *col_ref_total;

   /**
    * Action-less mesh visitor used purely to count the number of selected mesh
    * entities.
    */
   SmartPointer<MeshVisitor> _nullVisitor;
};
