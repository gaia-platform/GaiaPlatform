# automatically generated by the FlatBuffers compiler, do not modify

# namespace: GaiaPlatform

import flatbuffers

class NodeFields(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAsNodeFields(cls, buf, offset):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = NodeFields()
        x.Init(buf, n + offset)
        return x

    # NodeFields
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # NodeFields
    def Id(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            x = o + self._tab.Pos
            from .ID import ID
            obj = ID()
            obj.Init(self._tab.Bytes, x)
            return obj
        return None

    # NodeFields
    def Type(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int64Flags, o + self._tab.Pos)
        return -1

def NodeFieldsStart(builder): builder.StartObject(2)
def NodeFieldsAddId(builder, id): builder.PrependStructSlot(0, flatbuffers.number_types.UOffsetTFlags.py_type(id), 0)
def NodeFieldsAddType(builder, type): builder.PrependInt64Slot(1, type, -1)
def NodeFieldsEnd(builder): return builder.EndObject()
