
#ifndef QWY2_HEADER_CHUNK_
#define QWY2_HEADER_CHUNK_

#include "coords.hpp"
#include "mesh.hpp"
#include "shaders/classic/classic.hpp"
#include <glm/vec3.hpp>
#include <vector>
#include <array>
#include <unordered_map>
#include <optional>
#include <variant>
#include <future>
#include <tuple>
#include <fstream>

namespace qwy2
{

/* The length of the edges of the chunks, in blocks.
 * It must be odd, and should be at least 15 or something. */
extern unsigned int g_chunk_side;

/* Returns the coords of the block at the center of the chunk given by chunk_coords. */
BlockCoords chunk_center_coords(ChunkCoords chunk_coords);

/* Returns the coords of the block at the negativeward corner of the given chunk. */
BlockCoords chunk_most_negativeward_block_coords(ChunkCoords chunk_coords);

/* Returns the coords of the block at the positiveward corner of the given chunk. */
BlockCoords chunk_most_positiveward_block_coords(ChunkCoords chunk_coords);

/* Returns the block rect that contains exactly the blocks of the chunk given by chunk_coords. */
BlockRect chunk_block_rect(ChunkCoords chunk_coords);

/* Returns the block rect that contains exactly the blocks of the given chunk rect. */
BlockRect chunk_rect_block_rect(ChunkRect chunk_rect);

/* Returns the chunk-level coords of the chunk that contains the block at the given coords. */
ChunkCoords containing_chunk_coords(BlockCoords coords);

/* Returns the chunk-level coords of the chunk that contains the point at the given coords. */
ChunkCoords containing_chunk_coords(glm::vec3 coords);

/* Returns the chunk rect of all the chunks that intersect with the given block rect. */
ChunkRect containing_chunk_rect(BlockRect block_rect);

/* Grid of values, one value per block, for one chunk.
 * As this is mostly a pointer to the data, passing it by value does not copy the data. */
template<typename FieldValueType>
class ChunkField
{
public:
	using ValueType = FieldValueType;

public:
	ChunkCoords chunk_coords;
private:
	ValueType* data;

public:
	ChunkField();
	ChunkField(ChunkCoords chunk_coords);
	ChunkField(ChunkCoords chunk_coords, ValueType* data);
	~ChunkField();
	ValueType& operator[](BlockCoords coords);
	ValueType const& operator[](BlockCoords coords) const;

	/* Access raw field data. Access to values should be performed via [] operator,
	 * this is intended for use in stuff like write to disk. */
	ValueType* raw_data();
};

/* The PTG field (Plain Terrain Generation)
 * is the data generated by the first step of terrain generation. */
using PtgFieldValue = int;
using ChunkPtgField = ChunkField<PtgFieldValue>;

/* The PTT field (Plain Terrain Typing)
 * is the data generated by the second step of terrain generation. */
using BlockTypeId = unsigned int;
using PttFieldValue = BlockTypeId;
using ChunkPttField = ChunkField<PttFieldValue>;

class Block
{
public:
	BlockTypeId type_id;
public:
	bool is_air() const;
};

/* The B field is the actual grid of blocks contained by the chunk. */
using BFieldValue = Block;
using ChunkBField = ChunkField<BFieldValue>;

/* Holds one type of field for 3x3x3 cube of chunks.
 * It represents the neighborhood of the chunk at the center
 * and allows access to values as if it were a bigger kind of chunk. */
template<typename ChunkFieldType>
class ChunkNeighborhood
{
public:
	using FieldType = ChunkFieldType;
	using ValueType = typename FieldType::ValueType;

private:
	std::array<FieldType, 3*3*3> field_table;

public:
	ChunkNeighborhood();
	ValueType& operator[](BlockCoords coords);
	ValueType const& operator[](BlockCoords coords) const;

	ChunkRect chunk_rect() const;

	friend class ChunkGrid;
};

using ChunkMeshData = std::vector<VertexDataClassic>;

class Nature;

/* Generates the PTG field of the chunk at the given chunk-level coords.
 * Can be called in isolation, given that the nature is not modified before it returns. */
ChunkPtgField generate_chunk_ptg_field(
	ChunkCoords chunk_coords,
	Nature const& nature);

/* Generates the PTT field of the chunk at the given chunk-level coords,
 * using the PTG field of the nearby chunks.
 * Can be called in isolation, given that the nature is not modified before it returns. */
ChunkPttField generate_chunk_ptt_field(
	ChunkCoords chunk_coords,
	ChunkNeighborhood<ChunkPtgField> const chunk_neighborhood_ptg_field,
	Nature const& nature);

/* Generates the B field of the chunk at the given chunk-level coords,
 * using the PTT field of the nearby chunks.
 * Can be called in isolation, given that the nature is not modified before it returns. */
ChunkBField generate_chunk_b_field(
	ChunkCoords chunk_coords,
	ChunkNeighborhood<ChunkPttField> const chunk_neighborhood_ptt_field,
	Nature const& nature);

/* Generates the mesh data of the chunk at the given chunk-level coords,
 * using the B field of the nearby chunks.
 * Can be called in isolation, given that the nature is not modified before it returns. */
ChunkMeshData* generate_chunk_complete_mesh(
	ChunkCoords chunk_coords,
	ChunkNeighborhood<ChunkBField> const chunk_neighborhood_b_field,
	Nature const& nature);

/* TODO: See if passing stuff typed like `ChunkNeighborhood<ChunkPtgField>` to functions
 * performs an allocation and copy of the data or not. If it does, then this is something
 * to optimize (and beware the unloading of chunks while generating a neighbor which
 * may use-after-free across threads some data). */

template <typename ComponentType>
using ChunkComponentGrid = std::unordered_map<ChunkCoords, ComponentType, ChunkCoords::Hash>;

/* Handles the disk storage of a chunk's data.
 * TODO: Make this better. */
class ChunkDiskStorage
{
public:
	ChunkCoords chunk_coords;
public:
	/* Does the chunk actually has data stored on disk? */
	bool exist;

	std::string file_name;

public:
	ChunkDiskStorage();
	ChunkDiskStorage(ChunkCoords chunk_coords);
};

ChunkDiskStorage search_disk_for_chunk(ChunkCoords chunk_coords);
ChunkBField read_disk_chunk_b_field(ChunkCoords chunk_coords,
	ChunkDiskStorage& chunk_disk_storage);
void write_disk_chunk_b_field(ChunkCoords chunk_coords,
	ChunkDiskStorage& chunk_disk_storage, ChunkBField chunk_b_field);

class ChunkGrid
{
private:
public:
	ChunkComponentGrid<ChunkPtgField> ptg_field;
	ChunkComponentGrid<ChunkPttField> ptt_field;
	ChunkComponentGrid<ChunkBField> b_field;
	ChunkComponentGrid<Mesh<VertexDataClassic>> mesh;
	ChunkComponentGrid<ChunkDiskStorage> disk;

public:
	bool has_ptg_field(ChunkCoords chunk_coords) const;
	bool has_ptt_field(ChunkCoords chunk_coords) const;
	bool has_b_field(ChunkCoords chunk_coords) const;
	bool has_complete_mesh(ChunkCoords chunk_coords) const;
	bool has_disk_storage(ChunkCoords chunk_coords) const;

	bool has_ptg_field_neighborhood(ChunkCoords center_chunk_coords) const;
	bool has_ptt_field_neighborhood(ChunkCoords center_chunk_coords) const;
	bool has_b_field_neighborhood(ChunkCoords center_chunk_coords) const;
	
	ChunkNeighborhood<ChunkPtgField> const
		get_ptg_field_neighborhood(ChunkCoords center_chunk_coords) const;
	ChunkNeighborhood<ChunkPttField> const
		get_ptt_field_neighborhood(ChunkCoords center_chunk_coords) const;
	ChunkNeighborhood<ChunkBField> const
		get_b_field_neighborhood(ChunkCoords center_chunk_coords) const;

	bool block_is_air_or_unloaded(BlockCoords coords) const;

	void set_block(Nature const* nature,
		BlockCoords coords, BlockTypeId new_type_id);

	void write_all_to_disk();

	friend class ChunkGenerationManager;
};

class Nothing{};

using SomeChunkData =
	std::variant<
		ChunkPtgField, ChunkPttField, ChunkBField, ChunkMeshData*,
		ChunkDiskStorage, Nothing>;

enum class ChunkGeneratingStep
{
	PTG_FIELD,
	PTT_FIELD,
	DISK_SEARCH,
	DISK_READ,
	B_FIELD,
	MESH,
};

class ChunkGeneratingData
{
public:
	ChunkCoords chunk_coords;
	ChunkGeneratingStep step;
	std::future<SomeChunkData> future;
};

class ThreadPool;

/* An instance of this class should get to manage the process of generating chunks. */
class ChunkGenerationManager
{
public:
	/* The thread pool to which to give generation tasks. Not owend. */
	ThreadPool* thread_pool;

	/* The grid of chunks in which to generate chunks. Not owend. */
	ChunkGrid* chunk_grid;

	/* The block-level coords that should be at the center of the generated zone. */
	glm::vec3 generation_center;

	/* The radius (in blocks) of the generated zone. */
	float generation_radius;

	bool generation_enabled;

	bool load_save_enabled;

	/* The data that are undergoing generation (possibly in an other thread). */
	std::vector<std::optional<ChunkGeneratingData>> generating_data_vector;

public:
	ChunkGenerationManager();

	/* Should be called at every game loop iteration.
	 * This method is the core of `ChunkGenerationManager`, it manages generating thread jobs. */
	void manage(Nature const& nature);

private:
	/* Does the given chunk needs to have the given generation step to be started?
	 * Returning false means that the given step is already done or on its way. */
	bool needs_generation_step(ChunkCoords chunk_coords, ChunkGeneratingStep step) const;

	/* Some generation steps require other generation steps to be completed before.
	 * This function takes a generation step A that we would like to be able to do at some point,
	 * and returns a generation step B that can be done now and is required to get to A.
	 * If the given generation step A can be done now, then it is returned.
	 * The given generation step A should not be already done or on its way.
	 * It can return an empty optional due to some last required step to already be on its way,
	 * thus making waiting the only thing we can do in regards to the given step A. */
	std::optional<std::pair<ChunkCoords, ChunkGeneratingStep>> required_generation_step(
		ChunkCoords chunk_coords, ChunkGeneratingStep step) const;
};

} /* qwy2 */

#endif /* QWY2_HEADER_CHUNK_ */
