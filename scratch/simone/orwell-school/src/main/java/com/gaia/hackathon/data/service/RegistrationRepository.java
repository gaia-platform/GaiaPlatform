package com.gaia.hackathon.data.service;

import com.gaia.hackathon.data.entity.Registration;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.JpaSpecificationExecutor;

public interface RegistrationRepository extends JpaRepository<Registration, Void>, JpaSpecificationExecutor<Registration> {

}