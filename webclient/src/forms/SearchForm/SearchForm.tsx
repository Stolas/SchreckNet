// eslint-disable-next-line
import React, { Component } from "react";
import { connect } from 'react-redux';
import { Form, Field, reduxForm } from 'redux-form'

import Button from '@mui/material/Button';
import Divider from '@mui/material/Divider';
import Paper from '@mui/material/Paper';

import { InputField, CheckboxField } from 'components';
import { FormKey } from 'types';

import './SearchForm.css';

const SearchForm = ({ handleSubmit }) => (
  <Paper className="log-search">
    <Form className="log-search__form" onSubmit={handleSubmit}>
      <div className="log-search__form-item">
        <Field label="Username" name="userName" component={InputField} />
      </div>
      <div className="log-search__form-item">
        <Field label="IP Address" name="ipAddress" component={InputField} />
      </div>
      <div className="log-search__form-item">
        <Field label="Game Name" name="gameName" component={InputField} />
      </div>
      <div className="log-search__form-item">
        <Field label="GameID" name="gameId" component={InputField} />
      </div>
      <div className="log-search__form-item">
        <Field label="Message" name="message" component={InputField} />
      </div>
      <Divider />
      <div className="log-search__form-item log-location">
        <Field label="Rooms" name="logLocation.room" component={CheckboxField} />
        <Field label="Games" name="logLocation.game" component={CheckboxField} />
        <Field label="Chats" name="logLocation.chat" component={CheckboxField} />
      </div>
      <Divider />
      <div className="log-search__form-item">
        <span>Date Range: Coming Soon</span>
      </div>
      <Divider />
      <div className="log-search__form-item">
        <span>Maximum Results: 1000</span>
      </div>
      <Divider />
      <Button className="log-search__form-submit" color="primary" variant="contained" type="submit">
        Search Logs
      </Button>
    </Form>
  </Paper>
);

const propsMap = {
  form: FormKey.SEARCH_LOGS,
};

const mapStateToProps = () => ({

});

export default connect(mapStateToProps)(reduxForm(propsMap)(SearchForm));
