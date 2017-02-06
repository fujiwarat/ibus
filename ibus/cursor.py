# vim:set et sts=4 sw=4:
#
# ibus - The Input Bus
#
# Copyright (c) 2017 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2017 Red Hat, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place, Suite 330,
# Boston, MA  02111-1307  USA

__all__ = (
        "CursorLocation",
    )

import dbus
from exception import IBusException
from serializable import *
from attribute import AttrList

class CursorLocation(Serializable):
    __gtype_name__ = "PYIBusCursorLocation"
    __NAME__ = "IBusCursorLocation"
    def __init__ (self, x=0, y=0, width=0, height=0, display_name=""):
        super(CursorLocation, self).__init__()
        self.__x = x
        self.__y = y
        self.__width = width
        self.__height = height
        self.__display_name = display_name

    def get_x(self):
        return self.__x

    def get_y(self):
        return self.__y

    def get_width(self):
        return self.__width

    def get_height(self):
        return self.__height

    def get_display_name(self):
        return self.__display_name

    x            = property(get_x)
    y            = property(get_y)
    width        = property(get_width)
    height       = property(get_height)
    display_name = property(get_display_name)

    def serialize(self, struct):
        super(CursorLocation, self).serialize(struct)
        struct.append (dbus.Int32(self.__x))
        struct.append (dbus.Int32(self.__y))
        struct.append (dbus.Int32(self.__width))
        struct.append (dbus.Int32(self.__height))
        struct.append (dbus.String(self.__display_name))

    def deserialize(self, struct):
        super(CursorLocation, self).deserialize(struct)

        self.__x = struct.pop(0)
        self.__y = struct.pop(0)
        self.__width = struct.pop(0)
        self.__height = struct.pop(0)
        self.__display_name = struct.pop(0)

def test():
    cursor = CursorLocation(x=10, y=20, width=100, height=50,
                            display_name=":0.0")
    value = serialize_object(cursor)
    cursor = deserialize_object(value)

if __name__ == "__main__":
    test()
