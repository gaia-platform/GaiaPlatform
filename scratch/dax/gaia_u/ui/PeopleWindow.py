import pyforms
from pyforms.basewidget                import BaseWidget
from pyforms.controls       import ControlList
from People                 import People
from PersonWindow           import PersonWindow
from AddMenuFuntionality    import AddMenuFuntionality

class PeopleWindow(AddMenuFuntionality, People, BaseWidget):
    """
    This applications is a GUI implementation of the People class
    """

    def __init__(self):
        People.__init__(self)
        BaseWidget.__init__(self,'People window')

        #Definition of the forms fields
        self._peopleList    = ControlList('People', 
            plusFunction    = self.__addPersonBtnAction, 
            minusFunction   = self.__rmPersonBtnAction)

        self._peopleList.horizontalHeaders = ['First name', 'Middle name', 'Last name']

    def addPerson(self, person):
        """
        Reimplement the addPerson function from People class to update the GUI 
        everytime a new person is added.
        """
        super(PeopleWindow, self).addPerson(person)
        self._peopleList += [person._firstName, person._middleName, person._lastName]
        person.close() #After adding the person close the window

    def removePerson(self, index):
        """
        Reimplement the removePerson function from People class to update the GUI 
        everytime a person is removed.
        """
        super(PeopleWindow, self).removePerson(index)
        self._peopleList -= index

    def __addPersonBtnAction(self):
        """
        Add person button event. 
        """
        # A new instance of the PersonWindow is opened and shown to the user.
        win = PersonWindow() 
        win.parent = self
        win.show()

    def __rmPersonBtnAction(self):
        """
        Remove person button event
        """
        self.removePerson( self._peopleList.selected_row_index ) 

#Execute the application
if __name__ == "__main__":   pyforms.start_app( PeopleWindow )