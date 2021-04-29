package com.gaia.hackathon.data.service;

import com.gaia.hackathon.data.entity.Events;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.JpaSpecificationExecutor;

public interface EventsRepository extends JpaRepository<Events, Integer>, JpaSpecificationExecutor<Events> {

}