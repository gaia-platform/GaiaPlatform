class BasicScalarField:
    def __init__(self, name, type):
        self.name = name.lstrip("_")
        self.type = type

    def __str__(self):
        return str(self.type) + " " + self.name

    def __hash__(self):
        return hash(self.type + self.name)

class ComplexScalarField:
    def __init__(self, name, type):
        self.name = name.lstrip("_")
        self.type = type

    def __str__(self):
        return str(self.type) + " " + self.name

    def __hash__(self):
        return hash(self.type + self.name)

class BasicArrayField:
    def __init__(self, name, type):
        self.name = name.lstrip("_")
        self.type = type

    def __str__(self):
        return str(self.type) + " " + self.name

    def __hash__(self):
        return hash(self.type + self.name)

class ComplexArrayField:
    def __init__(self, name, type):
        self.name = name.lstrip("_")
        self.type = type

    def __str__(self):
        return str(self.type) + " " + self.name

    def __hash__(self):
        return hash(self.type + self.name)
