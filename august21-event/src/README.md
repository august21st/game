## 'Entity Data' specification
All types within these diagrams are equivalent  to the dataproto-cpp types,
with numerical types encoded in big endian format, and *_str / str types
being utf8 C-like string sequences, made up of flint-like length, and an
unterminated u8[].


### Used by:
 - `NodeShared::write_entity_data(String node_path, BufWriter& buffer)`
 - `NodeShared::write_entity_data(Node* node, BufWriter& buffer)`
 - `NodeShared::read_entity_data(BufWriter& buffer)`

### Diagram:
<pre style="overflow-x: auto; white-space: pre;">
EntityData:
  u8 object_type -----------
  |                        |
  v                        v
  ObjectType::INLINE_NODE  ObjectType::FILESYSTEM_NODE
  |                        |
  v                        v
  str class_str            str node_path_str
  |
  v
  flint property_count
  |
  v
  (for i = 0; i < property_count; i++)
    |
    v
    u8 prop_object_type -----------------------------------------------
    |                                |                                |
    v                                v                                |
    ObjectType::FILESYSTEM_RESOURCE  ObjectType::VARIANT              ObjectType::INLINE_NODE
    |                                |                                |
    v                                v                                v
    str resource_path_str            flint variant_size               EntityData
                                     |
                                     v
                                     arr[variant_size] variant_data
  |
  v
  flint child_count
  |
  v
  (for i = 0; i < child_count; i++)
    |
    v
    EntityData
</pre>

## 'Entity info' packet specification
### Diagram:
<pre style="overflow-x: auto; white-space: pre;">
u8 ServerPacket::ENTITIES_INFO
|
v
u16 entity_count
|
v
u32 entity_id
|
v
str parent_scene_str
|
v
EntityData
</pre>
