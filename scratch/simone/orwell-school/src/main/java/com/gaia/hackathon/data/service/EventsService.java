package com.gaia.hackathon.data.service;

import com.gaia.hackathon.data.entity.Events;
import com.gaia.hackathon.data.entity.Persons;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.vaadin.artur.helpers.CrudService;

@Service
public class EventsService extends CrudService<Events, Integer> {

    private final EventsRepository repository;

    public EventsService(@Autowired EventsRepository repository) {
        this.repository = repository;
    }

    @Override
    protected EventsRepository getRepository() {
        return repository;
    }

}
