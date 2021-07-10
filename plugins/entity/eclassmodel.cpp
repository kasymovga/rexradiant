/*
   Copyright (C) 2001-2006, William Joseph.
   All Rights Reserved.

   This file is part of GtkRadiant.

   GtkRadiant is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GtkRadiant is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GtkRadiant; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

///\file
///\brief Represents any entity which has a fixed size specified in its entity-definition and displays a model (e.g. ammo_bfg).
///
/// This entity displays the model specified in its entity-definition.
/// The "origin" and "angle" keys directly control the entity's local-to-parent transform.
/// The "rotation" key directly controls the entity's local-to-parent transform for Doom3 only.

#include "eclassmodel.h"

#include "cullable.h"
#include "renderable.h"
#include "editable.h"

#include "selectionlib.h"
#include "instancelib.h"
#include "transformlib.h"
#include "traverselib.h"
#include "entitylib.h"
#include "render.h"
#include "eclasslib.h"
#include "pivot.h"

#include "targetable.h"
#include "origin.h"
#include "angles.h"
#include "rotation.h"
#include "model.h"
#include "filters.h"
#include "namedentity.h"
#include "keyobservers.h"
#include "namekeys.h"
#include "modelskinkey.h"

#include "entity.h"

inline void read_aabb( AABB& aabb, const EntityClass& eclass ){
	aabb = aabb_for_minmax( eclass.mins, eclass.maxs );
}

class EclassModel :
	public Snappable
{
	MatrixTransform m_transform;
	EntityKeyValues m_entity;
	KeyObserverMap m_keyObservers;

	OriginKey m_originKey;
	Vector3 m_origin;
	AnglesKey m_anglesKey;
	Vector3 m_angles;
	RotationKey m_rotationKey;
	Float9 m_rotation;
	SingletonModel m_model;

	ClassnameFilter m_filter;
	NamedEntity m_named;
	NameKeys m_nameKeys;
	RenderablePivot m_renderOrigin;
	RenderableNamedEntity m_renderName;
	ModelSkinKey m_skin;

	AABB m_aabb_local;
	RenderableWireframeAABB m_aabb_wire;
	Matrix4 m_translation;

	Callback m_transformChanged;
	Callback m_evaluateTransform;

	void construct(){
		read_aabb( m_aabb_local, m_entity.getEntityClass() );

		default_rotation( m_rotation );

		m_keyObservers.insert( "classname", ClassnameFilter::ClassnameChangedCaller( m_filter ) );
		m_keyObservers.insert( Static<KeyIsName>::instance().m_nameKey, NamedEntity::IdentifierChangedCaller( m_named ) );
		if ( g_gameType == eGameTypeDoom3 ) {
			m_keyObservers.insert( "angle", RotationKey::AngleChangedCaller( m_rotationKey ) );
			m_keyObservers.insert( "rotation", RotationKey::RotationChangedCaller( m_rotationKey ) );
		}
		else
		{
			if( m_entity.getEntityClass().has_direction_key )
				m_keyObservers.insert( "angle", m_anglesKey.getGroupAngleChangedCallback() );
			else
				m_keyObservers.insert( "angle", m_anglesKey.getAngleChangedCallback() );
			if( m_entity.getEntityClass().has_angles_key )
				m_keyObservers.insert( "angles", m_anglesKey.getAnglesChangedCallback() );
		}
		m_keyObservers.insert( "origin", OriginKey::OriginChangedCaller( m_originKey ) );
	}

// vc 2k5 compiler fix
#if _MSC_VER >= 1400
public:
#endif

	void updateTransform(){
		m_translation = m_transform.localToParent() = matrix4_translation_for_vec3( m_origin );

		if ( g_gameType == eGameTypeDoom3 ) {
			matrix4_multiply_by_matrix4( m_transform.localToParent(), rotation_toMatrix( m_rotation ) );
		}
		else
		{
			matrix4_multiply_by_matrix4( m_transform.localToParent(), matrix4_rotation_for_euler_xyz_degrees( m_angles ) );
		}

		m_transformChanged();
	}
	typedef MemberCaller<EclassModel, &EclassModel::updateTransform> UpdateTransformCaller;

	void originChanged(){
		m_origin = m_originKey.m_origin;
		updateTransform();
	}
	typedef MemberCaller<EclassModel, &EclassModel::originChanged> OriginChangedCaller;
	void anglesChanged(){
		m_angles = m_anglesKey.m_angles;
		updateTransform();
	}
	typedef MemberCaller<EclassModel, &EclassModel::anglesChanged> AnglesChangedCaller;
	void rotationChanged(){
		rotation_assign( m_rotation, m_rotationKey.m_rotation );
		updateTransform();
	}
	typedef MemberCaller<EclassModel, &EclassModel::rotationChanged> RotationChangedCaller;

	void skinChanged(){
		scene::Node* node = m_model.getNode();
		if ( node != 0 ) {
			Node_modelSkinChanged( *node );
		}
	}
	typedef MemberCaller<EclassModel, &EclassModel::skinChanged> SkinChangedCaller;

public:

	EclassModel( EntityClass* eclass, scene::Node& node, const Callback& transformChanged, const Callback& evaluateTransform ) :
		m_entity( eclass ),
		m_originKey( OriginChangedCaller( *this ) ),
		m_origin( ORIGINKEY_IDENTITY ),
		m_anglesKey( AnglesChangedCaller( *this ), m_entity ),
		m_angles( ANGLESKEY_IDENTITY ),
		m_rotationKey( RotationChangedCaller( *this ) ),
		m_filter( m_entity, node ),
		m_named( m_entity ),
		m_nameKeys( m_entity ),
		m_renderName( m_named, g_vector3_identity ),
		m_skin( SkinChangedCaller( *this ) ),
		m_aabb_wire( m_aabb_local ),
		m_translation( g_matrix4_identity ),
		m_transformChanged( transformChanged ),
		m_evaluateTransform( evaluateTransform ){
		construct();
	}
	EclassModel( const EclassModel& other, scene::Node& node, const Callback& transformChanged, const Callback& evaluateTransform ) :
		m_entity( other.m_entity ),
		m_originKey( OriginChangedCaller( *this ) ),
		m_origin( ORIGINKEY_IDENTITY ),
		m_anglesKey( AnglesChangedCaller( *this ), m_entity ),
		m_angles( ANGLESKEY_IDENTITY ),
		m_rotationKey( RotationChangedCaller( *this ) ),
		m_filter( m_entity, node ),
		m_named( m_entity ),
		m_nameKeys( m_entity ),
		m_renderName( m_named, g_vector3_identity ),
		m_skin( SkinChangedCaller( *this ) ),
		m_aabb_wire( m_aabb_local ),
		m_translation( g_matrix4_identity ),
		m_transformChanged( transformChanged ),
		m_evaluateTransform( evaluateTransform ){
		construct();
	}

	InstanceCounter m_instanceCounter;
	void instanceAttach( const scene::Path& path ){
		if ( ++m_instanceCounter.m_count == 1 ) {
			m_filter.instanceAttach();
			m_entity.instanceAttach( path_find_mapfile( path.begin(), path.end() ) );
			m_entity.attach( m_keyObservers );
			m_model.modelChanged( m_entity.getEntityClass().modelpath() );
			m_skin.skinChanged( m_entity.getEntityClass().skin() );
		}
	}
	void instanceDetach( const scene::Path& path ){
		if ( --m_instanceCounter.m_count == 0 ) {
			m_skin.skinChanged( "" );
			m_model.modelChanged( "" );
			m_entity.detach( m_keyObservers );
			m_entity.instanceDetach( path_find_mapfile( path.begin(), path.end() ) );
			m_filter.instanceDetach();
		}
	}

	EntityKeyValues& getEntity(){
		return m_entity;
	}
	const EntityKeyValues& getEntity() const {
		return m_entity;
	}

	scene::Traversable& getTraversable(){
		return m_model.getTraversable();
	}
	Namespaced& getNamespaced(){
		return m_nameKeys;
	}
	Nameable& getNameable(){
		return m_named;
	}
	TransformNode& getTransformNode(){
		return m_transform;
	}
	ModelSkin& getModelSkin(){
		return m_skin.get();
	}

	void attach( scene::Traversable::Observer* observer ){
		m_model.attach( observer );
	}
	void detach( scene::Traversable::Observer* observer ){
		m_model.detach( observer );
	}

	void renderSolid( Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld, bool selected ) const {
		if ( selected || g_showBboxes ) {
			if( selected )
				m_renderOrigin.render( renderer, volume, localToWorld );

			renderer.PushState();
			renderer.Highlight( Renderer::ePrimitive, false );
			renderer.SetState( m_entity.getEntityClass().m_state_wire, Renderer::eWireframeOnly );
			renderer.SetState( m_entity.getEntityClass().m_state_wire, Renderer::eFullMaterials );
			renderer.addRenderable( m_aabb_wire, m_translation );
			renderer.PopState();
		}
		renderer.SetState( m_entity.getEntityClass().m_state_wire, Renderer::eWireframeOnly );
		if ( selected || ( g_showNames && ( volume.fill() || aabb_fits_view( m_aabb_local, volume.GetModelview(), volume.GetViewport(), g_showNamesRatio ) ) ) ) {
			m_renderName.render( renderer, volume, localToWorld, selected );
		}
	}
	void renderWireframe( Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld, bool selected ) const {
		renderSolid( renderer, volume, localToWorld, selected );
	}

	void translate( const Vector3& translation ){
		m_origin = origin_translated( m_origin, translation );
	}
	void rotate( const Quaternion& rotation ){
		if ( g_gameType == eGameTypeDoom3 ) {
			rotation_rotate( m_rotation, rotation );
		}
		else
		{
			m_angles = angles_rotated( m_angles, rotation );
			if( !m_entity.getEntityClass().has_angles_key ) // yaw angle only
				m_angles[0] = m_angles[1] = 0;
		}
	}
	void snapto( float snap ){
		m_originKey.m_origin = origin_snapped( m_originKey.m_origin, snap );
		m_originKey.write( &m_entity );
	}
	void revertTransform(){
		m_origin = m_originKey.m_origin;
		if ( g_gameType == eGameTypeDoom3 ) {
			rotation_assign( m_rotation, m_rotationKey.m_rotation );
		}
		else
		{
			m_angles = m_anglesKey.m_angles;
		}
	}
	void freezeTransform(){
		m_originKey.m_origin = m_origin;
		m_originKey.write( &m_entity );
		if ( g_gameType == eGameTypeDoom3 ) {
			rotation_assign( m_rotationKey.m_rotation, m_rotation );
			m_rotationKey.write( &m_entity );
		}
		else if( m_anglesKey.m_angles != m_angles )
		{
			m_anglesKey.m_angles = m_angles;
			m_anglesKey.write( &m_entity );
		}
	}
	void transformChanged(){
		revertTransform();
		m_evaluateTransform();
		updateTransform();
	}
	typedef MemberCaller<EclassModel, &EclassModel::transformChanged> TransformChangedCaller;
};

class EclassModelInstance : public TargetableInstance, public TransformModifier, public Renderable
{
	class TypeCasts
	{
		InstanceTypeCastTable m_casts;
	public:
		TypeCasts(){
			m_casts = TargetableInstance::StaticTypeCasts::instance().get();
			InstanceStaticCast<EclassModelInstance, Renderable>::install( m_casts );
			InstanceStaticCast<EclassModelInstance, Transformable>::install( m_casts );
			InstanceIdentityCast<EclassModelInstance>::install( m_casts );
		}
		InstanceTypeCastTable& get(){
			return m_casts;
		}
	};

	EclassModel& m_contained;
public:
	typedef LazyStatic<TypeCasts> StaticTypeCasts;

	STRING_CONSTANT( Name, "EclassModelInstance" );

	EclassModelInstance( const scene::Path& path, scene::Instance* parent, EclassModel& contained ) :
		TargetableInstance( path, parent, this, StaticTypeCasts::instance().get(), contained.getEntity(), *this ),
		TransformModifier( EclassModel::TransformChangedCaller( contained ), ApplyTransformCaller( *this ) ),
		m_contained( contained ){
		m_contained.instanceAttach( Instance::path() );

		StaticRenderableConnectionLines::instance().attach( *this );
	}
	~EclassModelInstance(){
		StaticRenderableConnectionLines::instance().detach( *this );

		m_contained.instanceDetach( Instance::path() );
	}
	void renderSolid( Renderer& renderer, const VolumeTest& volume ) const {
		m_contained.renderSolid( renderer, volume, Instance::localToWorld(), getSelectable().isSelected() );
	}
	void renderWireframe( Renderer& renderer, const VolumeTest& volume ) const {
		m_contained.renderWireframe( renderer, volume, Instance::localToWorld(), getSelectable().isSelected() );
	}

	void evaluateTransform(){
		if ( getType() == TRANSFORM_PRIMITIVE ) {
			m_contained.translate( getTranslation() );
			if( getRotation() != c_quaternion_identity ){
				m_contained.rotate( getRotation() );
			}

		}
	}
	void applyTransform(){
		m_contained.revertTransform();
		evaluateTransform();
		m_contained.freezeTransform();
	}
	typedef MemberCaller<EclassModelInstance, &EclassModelInstance::applyTransform> ApplyTransformCaller;
};

class EclassModelNode :
	public scene::Node::Symbiot,
	public scene::Instantiable,
	public scene::Cloneable,
	public scene::Traversable::Observer
{
	class TypeCasts
	{
		NodeTypeCastTable m_casts;
	public:
		TypeCasts(){
			NodeStaticCast<EclassModelNode, scene::Instantiable>::install( m_casts );
			NodeStaticCast<EclassModelNode, scene::Cloneable>::install( m_casts );
			NodeContainedCast<EclassModelNode, scene::Traversable>::install( m_casts );
			NodeContainedCast<EclassModelNode, Snappable>::install( m_casts );
			NodeContainedCast<EclassModelNode, TransformNode>::install( m_casts );
			NodeContainedCast<EclassModelNode, Entity>::install( m_casts );
			NodeContainedCast<EclassModelNode, Nameable>::install( m_casts );
			NodeContainedCast<EclassModelNode, Namespaced>::install( m_casts );
			NodeContainedCast<EclassModelNode, ModelSkin>::install( m_casts );
		}
		NodeTypeCastTable& get(){
			return m_casts;
		}
	};


	scene::Node m_node;
	InstanceSet m_instances;
	EclassModel m_contained;

	void construct(){
		m_contained.attach( this );
	}
	void destroy(){
		m_contained.detach( this );
	}

public:
	typedef LazyStatic<TypeCasts> StaticTypeCasts;

	scene::Traversable& get( NullType<scene::Traversable>){
		return m_contained.getTraversable();
	}
	Snappable& get( NullType<Snappable>){
		return m_contained;
	}
	TransformNode& get( NullType<TransformNode>){
		return m_contained.getTransformNode();
	}
	Entity& get( NullType<Entity>){
		return m_contained.getEntity();
	}
	Nameable& get( NullType<Nameable>){
		return m_contained.getNameable();
	}
	Namespaced& get( NullType<Namespaced>){
		return m_contained.getNamespaced();
	}
	ModelSkin& get( NullType<ModelSkin>){
		return m_contained.getModelSkin();
	}

	EclassModelNode( EntityClass* eclass ) :
		m_node( this, this, StaticTypeCasts::instance().get() ),
		m_contained( eclass, m_node, InstanceSet::TransformChangedCaller( m_instances ), InstanceSetEvaluateTransform<EclassModelInstance>::Caller( m_instances ) ){
		construct();
	}
	EclassModelNode( const EclassModelNode& other ) :
		scene::Node::Symbiot( other ),
		scene::Instantiable( other ),
		scene::Cloneable( other ),
		scene::Traversable::Observer( other ),
		m_node( this, this, StaticTypeCasts::instance().get() ),
		m_contained( other.m_contained, m_node, InstanceSet::TransformChangedCaller( m_instances ), InstanceSetEvaluateTransform<EclassModelInstance>::Caller( m_instances ) ){
		construct();
	}
	~EclassModelNode(){
		destroy();
	}
	void release(){
		delete this;
	}
	scene::Node& node(){
		return m_node;
	}

	void insert( scene::Node& child ){
		m_instances.insert( child );
	}
	void erase( scene::Node& child ){
		m_instances.erase( child );
	}

	scene::Node& clone() const {
		return ( new EclassModelNode( *this ) )->node();
	}

	scene::Instance* create( const scene::Path& path, scene::Instance* parent ){
		return new EclassModelInstance( path, parent, m_contained );
	}
	void forEachInstance( const scene::Instantiable::Visitor& visitor ){
		m_instances.forEachInstance( visitor );
	}
	void insert( scene::Instantiable::Observer* observer, const scene::Path& path, scene::Instance* instance ){
		m_instances.insert( observer, path, instance );
	}
	scene::Instance* erase( scene::Instantiable::Observer* observer, const scene::Path& path ){
		return m_instances.erase( observer, path );
	}
};

scene::Node& New_EclassModel( EntityClass* eclass ){
	return ( new EclassModelNode( eclass ) )->node();
}
