#ifndef slic3r_Utils_UndoRedo_hpp_
#define slic3r_Utils_UndoRedo_hpp_

#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <cassert>

#include <libslic3r/ObjectID.hpp>

namespace Slic3r {

class Model;

namespace GUI {
	class Selection;
    class GLGizmosManager;
} // namespace GUI

namespace UndoRedo {

struct Snapshot
{
	Snapshot(size_t timestamp) : timestamp(timestamp) {}
	Snapshot(const std::string &name, size_t timestamp, size_t model_id) : name(name), timestamp(timestamp), model_id(model_id) {}
	
	std::string name;
	size_t 		timestamp;
	size_t 		model_id;

	bool		operator< (const Snapshot &rhs) const { return this->timestamp < rhs.timestamp; }
	bool		operator==(const Snapshot &rhs) const { return this->timestamp == rhs.timestamp; }

	// The topmost snapshot represents the current state when going forward.
	bool 		is_topmost() const;
	// The topmost snapshot is not being serialized to the Undo / Redo stack until going back in time, 
	// when the top most state is being serialized, so we can redo back to the top most state.
	bool 		is_topmost_captured() const { assert(this->is_topmost()); return model_id > 0; }
};

// Excerpt of Slic3r::GUI::Selection for serialization onto the Undo / Redo stack.
struct Selection : public Slic3r::ObjectBase {
	unsigned char							mode;
	std::vector<std::pair<size_t, size_t>>	volumes_and_instances;
	template<class Archive> void serialize(Archive &ar) { ar(mode, volumes_and_instances); }
};

class StackImpl;

class Stack
{
public:
	// Stack needs to be initialized. An empty stack is not valid, there must be a "New Project" status stored at the beginning.
	// The first "New Project" snapshot shall not be removed.
	Stack();
	~Stack();

	// Store the current application state onto the Undo / Redo stack, remove all snapshots after m_active_snapshot_time.
    void take_snapshot(const std::string& snapshot_name, const Slic3r::Model& model, const Slic3r::GUI::Selection& selection, const Slic3r::GUI::GLGizmosManager& gizmos);

	// To be queried to enable / disable the Undo / Redo buttons at the UI.
	bool has_undo_snapshot() const;
	bool has_redo_snapshot() const;

	// Roll back the time. If time_to_load is SIZE_MAX, the previous snapshot is activated.
	// Undoing an action may need to take a snapshot of the current application state, so that redo to the current state is possible.
    bool undo(Slic3r::Model& model, const Slic3r::GUI::Selection& selection, Slic3r::GUI::GLGizmosManager& gizmos, size_t time_to_load = SIZE_MAX);

	// Jump forward in time. If time_to_load is SIZE_MAX, the next snapshot is activated.
    bool redo(Slic3r::Model& model, Slic3r::GUI::GLGizmosManager& gizmos, size_t time_to_load = SIZE_MAX);

	// Snapshot history (names with timestamps).
	// Each snapshot indicates start of an interval in which this operation is performed.
	// There is one additional snapshot taken at the very end, which indicates the current unnamed state.

	const std::vector<Snapshot>& snapshots() const;
	// Timestamp of the active snapshot. One of the snapshots of this->snapshots() shall have Snapshot::timestamp equal to this->active_snapshot_time().
	// The snapshot time indicates start of an operation, which is finished at the time of the following snapshot, therefore
	// the active snapshot is the successive snapshot. The same logic applies to the time_to_load parameter of undo() and redo() operations.
	size_t active_snapshot_time() const;

	// After load_snapshot() / undo() / redo() the selection is deserialized into a list of ObjectIDs, which needs to be converted
	// into the list of GLVolume pointers once the 3D scene is updated.
	const Selection& selection_deserialized() const;

private:
	friend class StackImpl;
	std::unique_ptr<StackImpl> 	pimpl;
};

}; // namespace UndoRedo
}; // namespace Slic3r

#endif /* slic3r_Utils_UndoRedo_hpp_ */
