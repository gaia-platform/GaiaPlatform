package com.gaia.hackathon.views.person;

import com.gaia.hackathon.data.entity.*;
import com.gaia.hackathon.data.service.*;
import com.gaia.hackathon.views.main.MasterDetailLayout;
import com.vaadin.flow.component.ItemLabelGenerator;
import com.vaadin.flow.component.button.Button;
import com.vaadin.flow.component.button.ButtonVariant;
import com.vaadin.flow.component.datepicker.DatePicker;
import com.vaadin.flow.component.dependency.CssImport;
import com.vaadin.flow.component.grid.Grid;
import com.vaadin.flow.component.grid.GridVariant;
import com.vaadin.flow.component.html.Div;
import com.vaadin.flow.component.notification.Notification;
import com.vaadin.flow.component.orderedlayout.HorizontalLayout;
import com.vaadin.flow.component.splitlayout.SplitLayout;
import com.vaadin.flow.component.textfield.TextField;
import com.vaadin.flow.data.binder.BeanValidationBinder;
import com.vaadin.flow.data.binder.ValidationException;
import com.vaadin.flow.data.renderer.LocalDateRenderer;
import com.vaadin.flow.data.renderer.TextRenderer;
import com.vaadin.flow.function.ValueProvider;
import com.vaadin.flow.router.PageTitle;
import com.vaadin.flow.router.Route;
import org.springframework.beans.factory.annotation.Autowired;
import org.vaadin.artur.helpers.CrudServiceDataProvider;

import java.time.Instant;
import java.time.LocalDate;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.time.format.FormatStyle;
import java.util.Optional;

@Route(value = "events", layout = MasterDetailLayout.class)
@PageTitle("Events")
@CssImport("./styles/views/masterdetail/master-detail-view.css")
public class EventsView extends Div {

    private Grid<Events> grid = new Grid<>(Events.class, false);

    private TextField name;
    private DatePicker startTime;
    private DatePicker endTime;
    private TextField room;
    private TextField teacher;

    private Button cancel = new Button("Cancel");
    private Button save = new Button("Save");

    private BeanValidationBinder<Events> binder;

    private Events event;

    public EventsView(@Autowired EventsService eventsService,
                      @Autowired RoomsRepository roomsRepository,
                      @Autowired StaffRepository staffRepository,
                      @Autowired BuildingsRepository buildingsRepository,
                      @Autowired PersonsRepository personsRepository) {
        setId("master-detail-view");
        // Create UI
        SplitLayout splitLayout = new SplitLayout();
        splitLayout.setSizeFull();

        createGridLayout(splitLayout);
        createEditorLayout(splitLayout);

        add(splitLayout);

        // Configure Grid
        grid.addColumn("name").setAutoWidth(true);
        grid.addColumn(new LocalDateRenderer<>(
                (ValueProvider<Events, LocalDate>) events
                        -> Instant.ofEpochMilli(events.getStartTime()).atZone(ZoneId.systemDefault()).toLocalDate(),
                DateTimeFormatter.ofLocalizedDate(FormatStyle.FULL)))
                .setHeader("Start Time");
        grid.addColumn(new LocalDateRenderer<>(
                (ValueProvider<Events, LocalDate>) events
                        -> Instant.ofEpochMilli(events.getEndTime()).atZone(ZoneId.systemDefault()).toLocalDate(),
                DateTimeFormatter.ofLocalizedDate(FormatStyle.FULL)))
                .setHeader("End Time");


        grid.addColumn(new TextRenderer<>((ItemLabelGenerator<Events>) events -> {
            Rooms r = roomsRepository.findById(events.getRoom().intValue()).get();
            Buildings b = buildingsRepository.findById(r.getBuilding().intValue()).get();
            return b.getBuildingName() + "." + r.getRoomName() + " " + r.getFloorNumber() + " floor";
        })).setHeader("Room");


        grid.addColumn(new TextRenderer<>((ItemLabelGenerator<Events>) events -> {
            Staff s = staffRepository.findById(events.getTeacher().intValue()).get();
            Persons p = personsRepository.findById(s.getPerson().intValue()).get();
            return p.getFirstName() + " " + p.getLastName();
        })).setHeader("Teacher");


        grid.setDataProvider(new CrudServiceDataProvider<>(eventsService));
        grid.addThemeVariants(GridVariant.LUMO_NO_BORDER);
        grid.setHeightFull();

        // when a row is selected or deselected, populate form
        grid.asSingleSelect().addValueChangeListener(event -> {
            if (event.getValue() != null) {
                Optional<Events> eventtFromBackend = eventsService.get(event.getValue().getGaiaId());
                // when a row is selected but the data is no longer available, refresh grid
                if (eventtFromBackend.isPresent()) {
                    populateForm(eventtFromBackend.get());
                } else {
                    refreshGrid();
                }
            } else {
                clearForm();
            }
        });


        // Configure Form
        binder = new BeanValidationBinder<>(Events.class);

        // Bind fields. This where you'd define e.g. validation rules

//        binder.bindInstanceFields(this);

        cancel.addClickListener(e -> {
            clearForm();
            refreshGrid();
        });

        save.addClickListener(e -> {
            try {
                if (this.event == null) {
                    this.event = new Events();
                }
                binder.writeBean(this.event);

                eventsService.update(this.event);
                clearForm();
                refreshGrid();
                Notification.show("Person details stored.");
            } catch (ValidationException validationException) {
                Notification.show("An exception happened while trying to store the person details.");
            }
        });

    }

    private void createEditorLayout(SplitLayout splitLayout) {
//        Div editorLayoutDiv = new Div();
//        editorLayoutDiv.setId("editor-layout");
//
//        Div editorDiv = new Div();
//        editorDiv.setId("editor");
//        editorLayoutDiv.add(editorDiv);
//
//        FormLayout formLayout = new FormLayout();
//        name = new TextField("First Name");
//        startTime = new DatePicker("Start Time");
//        endTime = new DatePicker("End Time");
//
//        private TextField name;
//        private DatePicker startTime;
//        private DatePicker endTime;
//        private TextField room;
//        private TextField teacher;
//
//        Component[] fields = new Component[]{name, startTime, endTime};
//
//        for (Component field : fields) {
//            ((HasStyle) field).addClassName("full-width");
//        }
//        formLayout.add(fields);
//        editorDiv.add(formLayout);
//        createButtonLayout(editorLayoutDiv);
//
//        splitLayout.addToSecondary(editorLayoutDiv);
    }

    private void createButtonLayout(Div editorLayoutDiv) {
        HorizontalLayout buttonLayout = new HorizontalLayout();
        buttonLayout.setId("button-layout");
        buttonLayout.setWidthFull();
        buttonLayout.setSpacing(true);
        cancel.addThemeVariants(ButtonVariant.LUMO_TERTIARY);
        save.addThemeVariants(ButtonVariant.LUMO_PRIMARY);
        buttonLayout.add(save, cancel);
        editorLayoutDiv.add(buttonLayout);
    }

    private void createGridLayout(SplitLayout splitLayout) {
        Div wrapper = new Div();
        wrapper.setId("grid-wrapper");
        wrapper.setWidthFull();
        splitLayout.addToPrimary(wrapper);
        wrapper.add(grid);
    }

    private void refreshGrid() {
        grid.select(null);
        grid.getDataProvider().refreshAll();
    }

    private void clearForm() {
        populateForm(null);
    }

    private void populateForm(Events value) {
        this.event = value;
        binder.readBean(this.event);

    }
}
