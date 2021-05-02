package com.gaia.hackathon.data.service;

import com.gaia.hackathon.data.entity.Persons;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.vaadin.artur.helpers.CrudService;

@Service
public class PersonService extends CrudService<Persons, Integer> {

    private final PersonsRepository repository;

    public PersonService(@Autowired PersonsRepository repository) {
        this.repository = repository;
    }


    @Override
    protected PersonsRepository getRepository() {
        return repository;
    }

}
